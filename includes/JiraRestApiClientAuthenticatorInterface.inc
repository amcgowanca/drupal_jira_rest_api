<?php
/**
 * @file
 * includes/JiraRestApiClientAuthenticatorInterface.inc
 */

/**
 * Jira REST API client authenticator interface.
 */
interface JiraRestApiClientAuthenticatorInterface {
  /**
   * Sets the JiraRestApiClient instance for this authenticator.
   *
   * @access public
   *
   * @param JiraRestApiClient $client
   *   The client instance.
   */
  public function setClient(JiraRestApiClient $client);

  /**
   * Returns the authenticator type machine name.
   *
   * @access public
   *
   * @return string
   *   The type defined in the information.
   */
  public function getType();

  /**
   * Returns the authenticator label.
   *
   * @access public
   *
   * @return string
   *   The label defined in the information.
   */
  public function getLabel();

  /**
   * Returns a boolean indicating if the client is authenticated.
   *
   * @access public
   *
   * @return bool
   *   Returns TRUE if the client is currently authenticated, otherwise FALSE.
   */
  public function isAuthenticated();

  /**
   * Returns a boolean indicating if the authentication is persistable.
   *
   * @access public
   *
   * @return boolean
   *   Returns TRUE if persistable, otherwise FALSE.
   */
  public function isPersistable();

  /**
   * Authenticate the client.
   *
   * @access public
   *
   * @param int $phase
   *   The phase for multi-step authentication handlers. Initial phase is the
   *   JIRA_REST_API_AUTH_PHASE_NONE. This parameter may be omitted if single
   *   step authentication is performed. Default value is
   *   JIRA_REST_API_AUTH_PHASE_NONE.
   *
   * @param array $context
   *   An array of contextual data for the authentication. Particularly this
   *   is used for session data stored in $_SESSION['jira_rest_api'] during
   *   multi-step authentication processes (e.g. OAuth 1.0) when using the
   *   authentication callback url.
   *
   * @return boolean
   *   Returns TRUE if authentication is successful, otherwise FALSE.
   */
  public function authenticate($phase = JIRA_REST_API_AUTH_PHASE_NONE, array $context = array());

  /**
   * Deauthenticate (revoke authentication) the client.
   *
   * @access public
   */
  public function deauthenticate();

  /**
   * Signs the HTTP request for authentication.
   *
   * @access public
   *
   * @param object $request
   *   The HTTP request object.
   *
   * @param array $parameters
   *   An array of parameters to sign and send as part of the HTTP request.
   *
   * @param string $resource_path
   *   The HTTP request's resource path for the request being signed.
   */
  public function signRequest(&$request, array $parameters = array(), $resource_path = NULL);

  /**
   * Tests the authentication of this client.
   *
   * @access public
   *
   * @return boolean
   *   Returns TRUE if authentication was successful, otherwise FALSE.
   */
  public function testAuthentication();
}
