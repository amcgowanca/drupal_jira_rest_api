<?php
/**
 * @file
 * jira_rest_api.api.inc
 */

/**
 * Register operations to perform on client instances.
 *
 * @return array
 *   An array of client operations.
 */
function hook_jira_rest_api_client_operations() {
  return array(
    'deauthenticate' => t('Deauthenticate selected clients'),
    'delete' => t('Delete selected clients'),
  );
}

/**
 * Allows modules to alter the registered operations.
 *
 * @param array $operations
 *   An array of operations information to alter.
 */
function hook_jira_rest_api_client_operations_alter(array &$operations) {
  $operations['delete'] = t('Delete clients');
}

/**
 * Allows modules to alter the HTTP Request object before invoking.
 *
 * @param object $request
 *   The HTTP request object to alter.
 *
 * @param JiraRestApiClient $client
 *   The Jira client instance making this request.
 */
function hook_jira_rest_api_request_alter(&$request, JiraRestApiClient $client) {
  $request->options['timeout'] = 60;
}

/**
 * Register available authenticators used for authenticating with Jira.
 *
 * @return array
 */
function hook_jira_rest_api_authenticator() {
  return array();
}

/**
 * Allow modules to perform operations when a client is inserted.
 *
 * @param JiraRestApiClient $client
 *   The client instance.
 */
function hook_jira_rest_api_client_insert(JiraRestApiClient $client) {
  watchdog('jira_rest_api', 'Created Jira REST API client instance %name (id: %cid)', array('%name' => $client->getLabel(), '%cid' => $client->getIdentifier()));
}

/**
 * Allow modules to perform operations when a client is updated.
 *
 * @param JiraRestApiClient $client
 *   The client instance.
 */
function hook_jira_rest_api_client_update(JiraRestApiClient $client) {
  watchdog('jira_rest_api', 'Updated Jira REST API client instance %name (id: %cid)', array('%name' => $client->getLabel(), '%cid' => $client->getIdentifier()));
}

/**
 * Allow modules to perform operations when a client instance is deleted.
 *
 * @param JiraRestApiClient $client
 *   The client instance.
 */
function hook_jira_rest_api_client_delete(JiraRestApiClient $client) {
  watchdog('jira_rest_api', 'Deleted Jira REST API client instance %name.', array('%name' => $client->getLabel()));
}
