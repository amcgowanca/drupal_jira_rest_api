<?php
/**
 * @file
 * jira_rest_api.pages.inc
 */

/**
 * Form callback for general settings form.
 */
function jira_rest_api_admin_settings_form($form, &$form_state) {
  $form['http_request_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('HTTP request settings'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['http_request_settings']['jira_rest_api_http_request_timeout'] = array(
    '#type' => 'textfield',
    '#title' => t('HTTP request timeout'),
    '#description' => t("The maximum number of seconds allowed for performing an HTTP request to the Jira REST API endpoint before timing out."),
    '#maxlength' => 6,
    '#size' => 20,
    '#default_value' => jira_rest_api_http_request_timeout(),
    '#element_validate' => array('element_validate_number'),
  );
  $form['http_request_settings']['jira_rest_api_http_request_max_redirects'] = array(
    '#type' => 'select',
    '#title' => t('HTTP request maximum redirects'),
    '#description' => t('The maximum number of redirects allowed when performing an HTTP request to the Jira REST API endpoint before stopping. It is recommended that zero redirects are allowed.'),
    '#options' => array(0 => t('Zero (0)'), 1 => t('One (1)'), 2 => t('Two (2)'), 3 => t('Three (3)'), 4 => t('Four (4)')),
    '#default_value' => jira_rest_api_http_request_max_redirects(),
  );
  return system_settings_form($form);
}

/**
 * Form callback for client listing form allowing bulk operations.
 */
function jira_rest_api_admin_clients_form($form, $form_state) {
  if (isset($form_state['values']['operation']) && 'delete' == $form_state['values']['operation']) {
    return jira_rest_api_admin_clients_delete_multiple_confirm($form, $form_state, array_filter($form_state['values']['clients']));
  }

  $operations = array();
  foreach (module_invoke_all('jira_rest_api_client_operations') as $op => $label) {
    $operations[$op] = $label;
  }
  drupal_alter('jira_rest_api_client_operations', $operations);

  $form['operations'] = array(
    '#type' => 'fieldset',
    '#title' => t('Operations'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['operations']['operation'] = array(
    '#type' => 'select',
    '#title' => t('Operation'),
    '#title_display' => 'invisible',
    '#options' => $operations,
    '#empty_option' => t('Select an action'),
    '#empty_value' => 'none',
  );
  $form['operations']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  $table_header = array(
    'label' => array('data' => t('Label'), 'field' => 'c.label'),
    'authenticator' => array('data' => t('Authenticator')),
    'authenticated' => array('data' => t('Authenticated')),
    'locked' => array('data' => t('Locked')),
    'operations' => array('data' => t('Operations')),
  );

  $table_rows = array();
  $locked_client_ids = array();

  $query = db_select('jira_rest_api_clients', 'c')
    ->extend('PagerDefault')
    ->extend('TableSort');
  $query->fields('c')
    ->limit(25)
    ->orderByHeader($table_header);
  $results = $query->execute();

  while ($row = $results->fetchObject()) {
    $client = JiraRestApiClient::fromObject($row);
    $table_rows[$client->getIdentifier()] = array(
      'label' => array(
        'data' => array(
          '#type' => 'link',
          '#title' => $client->getLabel(),
          '#href' => 'admin/config/services/jira-rest-api/clients/manage/' . $client->getName() . '/edit',
        ),
      ),
    );

    if ($client->isLocked()) {
      $locked_client_ids[] = $client->getIdentifier();
    }

    if ($authenticator = $client->getAuthenticator()) {
      if ($authenticator->isPersistable() && $authenticator->isAuthenticated()) {
        $is_authenticated_lang = t('Yes');
      }
      else if (!$authenticator->isPersistable()) {
        $is_authenticated_lang = t('No (not persistable)');
      }
      else {
        $is_authenticated_lang = t('No');
      }

      $table_rows[$client->getIdentifier()]['authenticator'] = array('data' => $authenticator->getLabel());
    }
    else {
      $is_authenticated_lang = t('No');
      $table_rows[$client->getIdentifier()]['authenticator'] = array('data' => t('not known'));
    }

    $table_rows[$client->getIdentifier()]['authenticated'] = array('data' => $is_authenticated_lang);
    $table_rows[$client->getIdentifier()]['locked'] = array('data' => $client->isLocked() ? t('Yes') : t('No'));

    $operations = array(
      'edit' => array(
        'title' => t('edit'),
        'href' => 'admin/config/services/jira-rest-api/clients/manage/' . $client->getName() . '/edit',
      ),
    );

    if (!$client->isLocked()) {
      $operations['delete'] = array(
        'title' => t('delete'),
        'href' => 'admin/config/services/jira-rest-api/clients/manage/' . $client->getName() . '/delete',
      );
    }

    if (1 < count($operations)) {
      $table_rows[$client->getIdentifier()]['operations'] = array(
        'data' => array(
          '#theme' => 'links__node_operations',
          '#links' => $operations,
          '#attributes' => array('class' => array('links', 'inline')),
        ),
      );
    }
    else {
      $link = reset($operations);
      $table_rows[$client->getIdentifier()]['operations'] = array(
        'data' => array(
          '#type' => 'link',
          '#title' => $link['title'],
          '#href' => $link['href'],
        ),
      );
    }
  }

  $form['clients'] = array(
    '#type' => 'tableselect',
    '#header' => $table_header,
    '#options' => $table_rows,
    '#empty' => t('There are currently no clients available.'),
  );

  foreach ($form['clients']['#options'] as $cid => $row) {
    if (in_array($cid, $locked_client_ids)) {
      $form['clients'][$cid]['#disabled'] = TRUE;
    }
  }

  $form['pager'] = array('#markup' => theme('pager'));

  $form['#validate'][] = 'jira_rest_api_admin_clients_form_validate';
  $form['#submit'][] = 'jira_rest_api_admin_clients_form_submit';

  return $form;
}

/**
 * Form validation callback for client listing form.
 */
function jira_rest_api_admin_clients_form_validate($form, &$form_state) {
  if (!is_array($form_state['values']['clients']) || !count(array_filter($form_state['values']['clients']))) {
    form_set_error('', t('No clients selected.'));
  }
}

/**
 * Form submit callback for client listing form.
 */
function jira_rest_api_admin_clients_form_submit($form, &$form_state) {
  // There are no client operations that need to be handled here yet, therefore
  // just mark the form as needing a rebuild and we off we go.
  $form_state['rebuild'] = TRUE;
}

/**
 * Form callback handler for deleting multiple clients.
 *
 * @see jira_rest_api_admin_clients_form().
 * @see confirm_form().
 */
function jira_rest_api_admin_clients_delete_multiple_confirm($form, &$form_state, array $client_ids = array()) {
  $form['clients'] = array(
    '#prefix' => '<ul>',
    '#suffix' => '</ul>',
    '#tree' => TRUE,
  );

  foreach ($client_ids as $cid => $value) {
    $label = db_query('SELECT label FROM {jira_rest_api_clients} WHERE cid = :cid AND locked = 0', array(':cid' => $cid))->fetchField();
    $form['clients'][$cid] = array(
      '#type' => 'hidden',
      '#value' => $cid,
      '#prefix' => '<li>',
      '#suffix' => check_plain($label) . "</li>\n",
    );
  }

  $form['operation'] = array(
    '#type' => 'hidden',
    '#value' => 'delete',
  );
  $form['#submit'][] = 'jira_rest_api_admin_clients_delete_multiple_confirm_submit';
  $confirm_question = format_plural(count($client_ids), 'Are you sure you want to delete this client?', 'Are you sure you want to delete these clients?');
  return confirm_form($form, $confirm_question, 'admin/config/services/jira-rest-api/clients', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}

/**
 * Form submit callback handler for deleting multiple clients.
 *
 * @see jira_rest_api_admin_clients_delete_multiple_confirm().
 */
function jira_rest_api_admin_clients_delete_multiple_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    foreach (array_keys($form_state['values']['clients']) as $cid) {
      if ($client = jira_rest_api_client_load_by_id($cid)) {
        $client->delete();
      }
    }
    $count = count($form_state['values']['clients']);
    drupal_set_message(format_plural($count, 'Deleted 1 client.', 'Deleted @count clients'));
  }
  $form_state['redirect'] = 'admin/config/services/jira-rest-api/clients';
}

/**
 * Page callback for creating a new client instance.
 *
 * @see jira_rest_api_admin_client_edit_form().
 */
function jira_rest_api_admin_client_add() {
  $client = new JiraRestApiClient();
  drupal_set_title(t('Create a new client'), PASS_THROUGH);
  return drupal_get_form('jira_rest_api_admin_client_edit_form', $client);
}

/**
 * Title callback for managing an existing client instance.
 *
 * @return string
 *   The title.
 */
function jira_rest_api_client_page_title(JiraRestApiClient $client) {
  return t('Manage %client', array('%client' => $client->getLabel()));
}

/**
 * Page callback for editing an existing client instance.
 *
 * @see jira_rest_api_admin_client_edit_form().
 */
function jira_rest_api_admin_client_edit(JiraRestApiClient $client) {
  drupal_set_title(t('Edit client %client', array('%client' => $client->getLabel())), PASS_THROUGH);
  return drupal_get_form('jira_rest_api_admin_client_edit_form', $client);
}

/**
 * Form callback for creating or editing a Jira REST API client instance.
 */
function jira_rest_api_admin_client_edit_form($form, &$form_state, JiraRestApiClient $client) {
  $is_new = !(bool) $client->getIdentifier();
  if (!$is_new && $authenticator = $client->getAuthenticator()) {
    if ($authenticator->isPersistable() && !$authenticator->isAuthenticated()) {
      drupal_set_message(t('This client is currently not authenticated.'), 'warning');
    }
  }

  if (!empty($form_state['values'])) {
    $values = $form_state['values'];
  }
  else {
    $values = array(
      'label' => $client->getLabel(),
      'name' => $client->getName(),
      'base_url' => $client->getBaseUrl(),
      'authenticator' => $client->getAuthTypeName(),
    ) + $client->getData();
  }

  $form['#client'] = $client;
  $form['#prefix'] = '<div id="form-wrapper">';
  $form['#suffix'] = '</div>';
  $form['label'] = array(
    '#type' => 'textfield',
    '#title' => t('Label'),
    '#description' => t('A human readable name for this client.'),
    '#default_value' => $values['label'],
    '#maxlength' => 255,
    '#required' => TRUE,
  );
  $form['name'] = array(
    '#type' => 'machine_name',
    '#title' => t('Machine name'),
    '#description' => t('A unique machine name for identifying this client (e.g. primary).'),
    '#maxlength' => 125,
    '#machine_name' => array(
      'exists' => 'jira_rest_api_client_machine_name_exists',
      'source' => array('label'),
      'replace_pattern' => '[^a-z0-9-]+',
      'replace' => '-',
    ),
    '#required' => TRUE,
    '#disabled' => !$is_new ? TRUE : FALSE,
    '#default_value' => $values['name'],
  );
  $form['base_url'] = array(
    '#type' => 'textfield',
    '#title' => t('Base Jira URL'),
    '#description' => t('The base URL of the Jira instance that this client authenticates with.'),
    '#required' => TRUE,
    '#default_value' => $values['base_url'],
  );

  $authenticators = jira_rest_api_authenticators();
  $authenticator_options = array();
  foreach ($authenticators as $authenticator_info) {
    $authenticator_options[$authenticator_info['name']] = $authenticator_info['label'];
  }

  $form['authenticator'] = array(
    '#type' => 'select',
    '#title' => t('Authentication method'),
    '#description' => t('The method of authenticating with the Jira instance.'),
    '#required' => TRUE,
    '#options' => $authenticator_options,
    '#default_value' => $values['authenticator'],
    '#ajax' => array(
      'callback' => '_jira_rest_api_admin_client_edit_form_authenticator_ajax',
      'wrapper' => 'form-wrapper',
    ),
  );

  if (1 == count($authenticator_options)) {
    $authenticator_name = key($authenticator_options);
    $authenticator = $authenticators[$authenticator_name];
    $form['authenticator']['#default_value'] = $authenticator['name'];
    $form['authenticator']['#disabled'] = TRUE;
  }
  else if (!empty($values['authenticator'])) {
    $authenticator_name = $values['authenticator'];
    if (isset($authenticators[$authenticator_name])) {
      $authenticator = $authenticators[$authenticator_name];
      $form['authenticator']['#default_value'] = $authenticator['name'];
      if (!$is_new) {
        $form['authenticator']['#disabled'] = TRUE;
      }
    }
  }

  if (isset($authenticator) && !empty($authenticator['form callback'])) {
    $form['authenticator_settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Authenticator settings'),
      '#collapsed' => FALSE,
      '#collapsible' => FALSE,
    );

    $file = $authenticator['file path'] . '/' . $authenticator['file'];
    if (file_exists($file)) {
      include_once $file;
    }

    $form_callback = $authenticator['form callback'];
    if (function_exists($form_callback)) {
      $return = $form_callback($form, $form_state, $values);
      if (FALSE === $return) {
        unset($form['authenticator_settings']);
      }
    }
  }

  if (!$is_new) {
    $form['cid'] = array(
      '#type' => 'value',
      '#value' => $client->getIdentifier(),
    );
  }

  $form['actions'] = array(
    '#type' => 'container',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  if (!$is_new) {
    if ($authenticator = $client->getAuthenticator()) {
      if ($authenticator->isPersistable()) {
        if ($authenticator->isAuthenticated()) {
          $form['actions']['deauthenticate'] = array(
            '#type' => 'submit',
            '#value' => t('Deauthenticate'),
          );
        }
        else {
          $form['actions']['authenticate'] = array(
            '#type' => 'submit',
            '#value' => t('Authenticate'),
          );
        }
      }
      else {
        $form['actions']['authenticate_test'] = array(
          '#type' => 'submit',
          '#value' => t('Test authentication'),
        );
      }
    }

    if (!$client->isLocked()) {
      $form['actions']['delete'] = array(
        '#type' => 'submit',
        '#value' => t('Delete'),
      );
    }
  }

  $form['#validate'][] = 'jira_rest_api_admin_client_edit_form_validate';
  $form['#submit'][] = 'jira_rest_api_admin_client_edit_form_submit';

  return $form;
}

/**
 * Form validate callback for creating or editing an existing client.
 *
 * @see jira_rest_api_admin_client_edit_form().
 */
function jira_rest_api_admin_client_edit_form_validate($form, &$form_state) {
  $authenticators = jira_rest_api_authenticators();
  if (isset($authenticators[$form_state['values']['authenticator']])) {
    $authenticator = $authenticators[$form_state['values']['authenticator']];
    $file = $authenticator['file path'] . '/' . $authenticator['file'];
    if (file_exists($file)) {
      include_once $file;
    }

    if (isset($authenticator['form validate callback'])) {
      $form_validate_callback = $authenticator['form validate callback'];
    }
    elseif (isset($authenticator['form callback'])) {
      $form_validate_callback = $authenticator['form callback'] . '_validate';
    }

    if (function_exists($form_validate_callback)) {
      $form_validate_callback($form, $form_state);
    }
  }
}

/**
 * Form submit callback for creating or editing an existing client.
 *
 * @see jira_rest_api_admin_client_edit_form().
 */
function jira_rest_api_admin_client_edit_form_submit($form, &$form_state) {
  if (t('Delete') == $form_state['values']['op'] && $client = $form['#client']) {
    $form_state['redirect'] = 'admin/config/services/jira-rest-api/clients';
    $client->delete();
    return;
  }

  $values = $form_state['values'];
  if (empty($values['cid'])) {
    $is_new = TRUE;
    // We cannot use the $form['#client'] instance here as we need to create an
    // instance of the JiraRestApiClient with the authenticator type name.
    $client = new JiraRestApiClient(NULL, NULL, NULL, NULL, $values['authenticator']);
  }
  else {
    $client = $form['#client'];
  }

  $client->setLabel($values['label']);
  $client->setName($values['name']);
  $client->setBaseUrl($values['base_url']);

  $data = array();
  foreach ($values as $key => $value) {
    if (!in_array($key, array('submit', 'cid', 'label', 'name', 'base_url', 'authenticator', 'op', 'form_build_id', 'form_token', 'form_id', 'deauthenticate', 'authenticate', 'authenticate_test', 'delete'))) {
      $client->$key = $value;
    }
  }

  switch ($form_state['values']['op']) {
    case t('Save'):
      if ($client->save()) {
        drupal_set_message(t('Successfully saved Jira REST API client %client.', array(
          '%client' => $client->getLabel(),
        )));
        $form_state['redirect'] = 'admin/config/services/jira-rest-api/clients';
      }
      break;

    case t('Authenticate'):
      if ($client->save()) {
        $authentication_context = array(
          'redirect' => url('admin/config/services/jira-rest-api/clients/manage/' . $client->getName()),
        );
        _jira_rest_api_authentication_flow($client, JIRA_REST_API_AUTH_PHASE_NONE, $authentication_context);
      }
      break;

    case t('Deauthenticate'):
      if ($authenticator = $client->getAuthenticator()) {
        $authenticator->deauthenticate();
        jira_rest_api_clients_static_reset();
      }
      drupal_set_message(t('You have successfully deauthenticated the %label Jira REST API client.', array(
        '%label' => $client->getLabel(),
      )));
      break;

    case t('Test authentication'):
      if ($client->save()) {
        // The test authentication callback sets messages which we will display
        // and should provide sufficient information on whether or not
        // authentication for this client is working.
        _jira_rest_api_admin_client_test($client);
      }
      else {
        drupal_set_message(t('An error occurred while saving the Jira REST API client information. As a result, we were unable to test the authentication.'), 'error');
      }
      break;
  }
}

/**
 * Ajax form callback for the authenticator element select change.
 */
function _jira_rest_api_admin_client_edit_form_authenticator_ajax($form, &$form_state) {
  return $form;
}

/**
 * Form callback for deleting a single client instance.
 */
function jira_rest_api_admin_client_delete($form, &$form_state, JiraRestApiClient $client) {
  if ($client->isLocked()) {
    drupal_set_message(t('You cannot delete Jira REST API client %name because it is locked.', array('%name' => $client->getLabel())), 'notice');
    return drupal_goto('admin/config/services/jira-rest-api/clients');
  }

  $form['#client'] = $client;
  $form['#submit'][] = 'jira_rest_api_admin_client_delete_submit';
  return confirm_form($form, t('Are you sure you want to delete this client named %label?', array('%label' => $client->getLabel())), 'admin/config/services/jira-rest-api/clients', t('This action cannot be undone.'), t('Delete'), t('Cancel'));
}

/**
 * Form submit handler for deleting a single client instance.
 */
function jira_rest_api_admin_client_delete_submit($form, &$form_state) {
  if (isset($form['#client'])) {
    $form['#client']->delete();
    drupal_set_message(t('You have successfully deleted Jira REST API client %label.', array('%label' => $form['#client']->getLabel())));
  }
  $form_state['redirect'] = 'admin/config/services/jira-rest-api/clients';
}

/**
 * Page handler for testing authentication of a client.
 */
function jira_rest_api_admin_client_test(JiraRestApiClient $client, $callback = FALSE) {
  _jira_rest_api_admin_client_test($client);
  $destination = drupal_get_destination();
  if (!empty($destination['destination'])) {
    return drupal_goto($destination['destination']);
  }
  return drupal_goto('admin/config/services/jira-rest-api/clients');
}

/**
 * Performs actual authentication test and sets appropriate messaging.
 *
 * @see jira_rest_api_admin_client_test().
 */
function _jira_rest_api_admin_client_test(JiraRestApiClient $client) {
  if ($authenticator = $client->getAuthenticator()) {
    if ($authenticator->isPersistable()) {
      if (!$authenticator->isAuthenticated()) {
        if ($authenticator->authenticate()) {
          drupal_set_message(t('Jira REST API client %name has successfully been authenticated.', array(
            '%name' => $client->getLabel(),
          )));
        }
        else {
          drupal_set_message(t('Unable to authenticate Jira REST API client %name.', array(
            '%name' => $client->getLabel(),
          )), 'error');
        }
      }
      else {
        $run_test_auth = TRUE;
      }
    }
    else {
      $run_test_auth = TRUE;
    }

    if (isset($run_test_auth) && $run_test_auth) {
      if ($authenticator->testAuthentication()) {
        drupal_set_message(t('Jira REST API client %name is authenticated.', array(
          '%name' => $client->getLabel(),
        )));
      }
      else {
        drupal_set_message(t('Jira REST API client %name cannot authenticate with Jira instance %base_url. Please check your authentication details.', array(
          '%name' => $client->getLabel(),
          '%base_url' => $client->getBaseUrl(),
        )), 'warning');
      }
    }
  }
}
