<?php
/**
 * @file
 * includes/jira_rest_api.authenticators.inc
 */

/**
 * Form callback for the OAuth 1.0 settings form for client configuration.
 */
function jira_rest_api_authenticator_oauth_1_0_client_settings_form(&$form, &$form_state, $values = array()) {
  $settings = &$form['authenticator_settings'];
  $settings['consumer_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Jira application consumer key'),
    '#description' => t('The consumer key used for authenticating with the Jira instance at the specified base URL.'),
    '#required' => TRUE,
    '#maxlength' => 255,
    '#default_value' => isset($values['consumer_key']) ? $values['consumer_key'] : '',
  );
  $settings['private_cert'] = array(
    '#type' => 'textfield',
    '#title' => t('Private certificate file'),
    '#description' => t('The absolute system file path to the private certificate file (.pem) used for signing the HTTP requests with.'),
    '#required' => TRUE,
    '#maxlength' => 512,
    '#default_value' => isset($values['private_cert']) ? $values['private_cert'] : '',
  );
  return TRUE;
}

/**
 * Form validation callback for OAuth 1.0 settings form.
 */
function jira_rest_api_authenticator_oauth_1_0_client_settings_form_validate(&$form, &$form_state) {
  if (!empty($form_state['values']['authenticator_settings']['private_cert'])) {
    $private_cert_file = $form_state['values']['authenticator_settings']['private_cert'];
    if (!file_exists($private_cert_file) || !is_file($private_cert_file)) {
      form_set_error('authenticator_settings][private_cert', t('The specified private certificate file does not exist.'));
    }
  }
}

/**
 * Form callback for the HTTP Basic Auth settings form for client configuration.
 */
function jira_rest_api_authenticator_http_basic_client_settings_form(&$form, &$form_state, $values = array()) {
  $settings = &$form['authenticator_settings'];
  $settings['username'] = array(
    '#type' => 'textfield',
    '#title' => t('Jira account username'),
    '#description' => t('The username to use for authenticating with the Jira instance at the specified based URL.'),
    '#required' => TRUE,
    '#maxlength' => 255,
    '#default_value' => isset($values['username']) ? $values['username'] : '',
  );
  $settings['password'] = array(
    '#type' => 'password',
    '#title' => t('Jira account password'),
    '#description' => t('The password of the Jira account user.'),
    '#maxlength' => 255,
    '#required' => TRUE,
    '#default_value' => isset($values['password']) ? $values['password'] : '',
  );
  return TRUE;
}

/**
 * OAuth 1.0 authentication class handler for Jira authentication.
 */
class JiraRestApiClientAuthenticatorOAuth extends JiraRestApiClientAuthenticator {
  /**
   * {@inheritdoc}
   */
  public function isAuthenticated() {
    if ($client = $this->getClient()) {
      if (!empty($client->oauth_token) && !empty($client->oauth_token_secret)) {
        return TRUE;
      }
      $this->resetOAuthTokenInfo();
    }
    return FALSE;
  }

  /**
   * {@inheritdoc}
   *
   * @throws Exception
   *   Thrown if an error occurs during authentication if:
   *     * The client does not exist for this authenticator.
   *     * During authentication if the `oauth_verifier` query parameter is not
   *       set when returned from Jira authorization url.
   */
  public function authenticate($phase = JIRA_REST_API_AUTH_PHASE_NONE, array $context = array()) {
    $client = $this->getClient();
    if (!$client) {
      throw new Exception(t('Attempt to authenticate with authenticator %name but no client exists.', array('%name' => $this->getLabel())));
    }

    $return = FALSE;
    switch ($phase) {
      case JIRA_REST_API_AUTH_PHASE_CALLBACK:
        $query_parameters = drupal_get_query_parameters();
        if (!isset($query_parameters['oauth_verifier'])) {
          throw new Exception(t('No `oauth_verifier` parameter specified in the callback.'));
        }

        $client->oauth_verifier = $query_parameters['oauth_verifier'];
        $response = jira_rest_api_request($client, 'plugins/servlet/oauth/access-token', array(
          'oauth_callback' => jira_rest_api_authentication_callback_url($client),
          'oauth_verifier' => $client->oauth_verifier,
        ), JIRA_REST_API_HTTP_POST, FALSE);
        if (200 == $response->code) {
          $client->oauth_token = $response->data['oauth_token'];
          $client->oauth_token_secret = $response->data['oauth_token_secret'];
          $client->oauth_expires_in = REQUEST_TIME + $response->data['oauth_expires_in'];
          $client->oauth_session_handle = $response->data['oauth_session_handle'];
          $client->oauth_authorization_expires_in = REQUEST_TIME + $response->data['oauth_authorization_expires_in'];
          $client->save();
          $return = TRUE;
        }
        break;
      case JIRA_REST_API_AUTH_PHASE_NONE:
      default:
        if (!empty($client->oauth_token) && !empty($client->oauth_token_secret) && !empty($client->oauth_token_expires) && REQUEST_TIME < $client->oauth_expires_in) {
          $ready_for_authorization = TRUE;
        }
        else {
          $response = jira_rest_api_request($client, 'plugins/servlet/oauth/request-token', array(
            'oauth_callback' => jira_rest_api_authentication_callback_url($client),
          ), JIRA_REST_API_HTTP_POST, FALSE);

          if (200 == $response->code) {
            if (empty($response->data['oauth_token']) || empty($response->data['oauth_token_secret']) || empty($response->data['oauth_callback_confirmed'])) {
              throw new Exception(t("Attempted to retrieve request tokens from Jira's REST API for client %client was unsuccessful.", array('%client' => $client->getLabel())));
            }
            // Store the retrieved tokens temporarily until the real authorized
            // tokens have been retrieved.
            $client->oauth_token = $response->data['oauth_token'];
            $client->oauth_token_secret = $response->data['oauth_token_secret'];
            $client->oauth_expires_in = REQUEST_TIME + 600;
            $client->save();
            $ready_for_authorization = TRUE;
            $requested_tokens = TRUE;
          }
        }

        if (isset($ready_for_authorization)) {
          // While we will never actually exit this function due to the
          // `drupal_exit()` call, for the sake of understanding we will set the
          // return variable to TRUE indicating that this is successful.
          $return = TRUE;
          // Set the `jira_rest_api` session variable so that we tell the
          // authentication callback that we are authorizing the return from
          // the Jira authentication screen.
          $_SESSION['jira_rest_api'] = $context;
          // Authorize the token received during the request-token API call.
          drupal_goto($this->getAuthorizationUrl());
          drupal_exit();
        }
        break;
    }
    return $return;
  }

  /**
   * {@inheritdoc}
   */
  public function deauthenticate() {
    if ($client = $this->getClient()) {
      $this->resetOAuthTokenInfo();
    }
  }

  /**
   * {@inheritdoc}
   */
  public function signRequest(&$request, array $parameters = array(), $resource_path = NULL) {
    $client = $this->getClient();
    if (!$client) {
      throw new Exception(t('Attempted to sign HTTP request with authenticator %label but no client specified.', array('%label' => $this->getLabel())));
    }

    if (!isset($client->consumer_key)) {
      throw new Exception(t('Attempted to sign HTTP request with the %client Jira REST API client but the client does not contain a consumer key.', array('%client' => $client->getLabel())));
    }

    $callback_url = jira_rest_api_authentication_callback_url($client);
    $consumer = new OAuthConsumer($client->consumer_key, '', $callback_url);
    $oauth_token = NULL;
    if (!empty($client->oauth_token) && !empty($client->oauth_token_secret)) {
      $oauth_token = new OAuthToken($client->oauth_token, $client->oauth_token_secret);
    }

    $oauth_request = OAuthRequest::from_consumer_and_token($consumer, $oauth_token, $request->options['method'], $request->url, $parameters);
    $signature_method = new JiraRestApiOAuthSignatureMethod_RSA_SHA1($client);
    $oauth_request->sign_request($signature_method, $consumer, $token);
    $request->url = $oauth_request->to_url();
  }

  /**
   * {@inheritdoc}
   */
  public function isPersistable() {
    return TRUE;
  }

  /**
   * {@inheritdoc}
   */
  public function testAuthentication() {
    if ($this->isAuthenticated() && $client = $this->getClient()) {
      $response = jira_rest_api_request($client, 'myself', array(), JIRA_REST_API_HTTP_GET);
      if (200 == $response->code) {
        return TRUE;
      }
    }
    return FALSE;
  }

  /**
   * Returns the authorization URL which a user is redirected too.
   *
   * @access protected
   *
   * @return string
   *   Returns the URL to redirect a user to for authorization. If the client
   *   was not set and cannot be retrieved, NULL is returned.
   */
  protected function getAuthorizationUrl() {
    if ($client = $this->getClient()) {
      $url = rtrim($client->getBaseUrl(), '/') . '/plugins/servlet/oauth/authorize';
      $url .= '?' . drupal_http_build_query(array(
          'oauth_token' => $client->oauth_token,
        ));
      return $url;
    }
    return NULL;
  }

  /**
   * Removes all client data items which begin with `oauth_`.
   *
   * @access protected
   */
  protected function resetOAuthTokenInfo() {
    if ($client = $this->getClient()) {
      $changed = FALSE;
      $client_data = $client->getData();
      foreach ($client_data as $key => $v) {
        // If this data key begins with `oauth_`, assume that we created it and
        // therefore it should be removed from the client's data.
        if (0 === strpos($key, 'oauth_')) {
          unset($client_data[$key]);
          $changed = TRUE;
        }
      }

      if ($changed) {
        $client->setData($client_data);
        $client->save();
      }
    }
  }
}

/**
 * HTTP Basic Auth authentication class handler for Jira authentication.
 */
class JiraRestApiClientAuthenticatorHttpBasicAuth extends JiraRestApiClientAuthenticator {
  /**
   * {@inheritdoc}
   */
  public function isAuthenticated() {
    if ($client = $this->getClient()) {
      return !empty($client->username) && !empty($client->password) ? TRUE : FALSE;
    }
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function authenticate($phase = JIRA_REST_API_AUTH_PHASE_NONE, array $context = array()) {
    $return = FALSE;
    if ($client = $this->getClient()) {
      if (!empty($client->username) && !empty($client->password)) {
        $expires = isset($client->last_test_authentication) ? ($client->last_test_authentication + 120) : 0;
        if (REQUEST_TIME > $expires) {
          try {
            if ($response = $client->request('myself', array(), JIRA_REST_API_HTTP_GET)) {
              if ($response->code) {
                $client->last_test_authentication = REQUEST_TIME;
                $client->save();
                $return = TRUE;
              }
            }
          }
          catch (Exception $exception) {
            jira_rest_api_log_exception($exception);
            throw $exception;
          }
        }
        else {
          $return = TRUE;
        }
      }
    }
    return $return;
  }

  /**
   * {@inheritdoc}
   */
  public function deauthenticate() {
    if ($client = $this->getClient()) {
      if (isset($client->last_test_authentication)) {
        unset($client->last_test_authentication);
        $save_client = TRUE;
      }

      if (isset($client->password) && !empty($client->password)) {
        $client->password = NULL;
        $save_client = TRUE;
      }

      if (isset($save_client)) {
        $client->save();
      }
      return TRUE;
    }
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function signRequest(&$request, array $parameters = array(), $resource_path = NULL) {
    $client = $this->getClient();
    if (!$client) {
      throw new Exception(t('Attempted to sign a request with %name but no client is specified.', array('%name' => $this->getLabel())));
    }
    // Verify that the client's username and password has been set prior to
    // continuing. Should either of them be empty, throw an Exception and halt
    // the signing.
    if (empty($client->username) || empty($client->password)) {
      throw new Exception(t('Unable to sign request for Jira REST API client %name as the username or password is not specified.', array('%name' => $client->getLabel())));
    }
    // HTTP Basic authentication only requires the `Authorization` header to be
    // in the username:password format and encoded in base64 encryption.
    $request->options['headers']['Authorization'] = 'Basic ' . drupal_base64_encode($client->username . ':' . $client->password);
  }

  /**
   * {@inheritdoc}
   */
  public function isPersistable() {
    return FALSE;
  }

  /**
   * {@inheritdoc}
   */
  public function testAuthentication() {
    // The method for authenticating via HTTP Basic authorization is by sending
    // the Authorization header via a request. Therefore, to `authenticate` we
    // simply test and ensure the credentials are valid via an API request to
    // the "myself" endpoint. Do this for testing as well.
    return $this->authenticate();
  }
}
