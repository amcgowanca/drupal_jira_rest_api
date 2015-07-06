<?php
/**
 * @file
 * includes/JiraRestApiOAuthSignatureMethod_RSA_SHA1.inc
 */

/**
 * Overrides OAuthSignatureMethod_RSA_SHA1 to provide specification.
 */
class JiraRestApiOAuthSignatureMethod_RSA_SHA1 extends OAuthSignatureMethod_RSA_SHA1 {
  /**
   * @var JiraRestApiClient
   *   The client instance.
   */
  private $client = NULL;

  /**
   * Constructor for creating new instances.
   *
   * @param JiraRestApiClient $client
   *   The client instance.
   */
  public function __construct(JiraRestApiClient $client) {
    $this->client = $client;
  }

  /**
   * Method not implemented.
   *
   * @throws Exception
   *   Throws exception as method is not implemented.
   */
  public function fetch_public_cert(&$request) {
    throw new Exception(t('%method not implemented.', array('%method' => __METHOD__)));
  }

  /**
   * Retrieves the private certificate file path.
   *
   * @return string
   *   The private certificate file path.
   *
   * @throws Exception
   *   Throws Exception if the client is not specified and or private cert file
   *   does not exist.
   */
  public function fetch_private_cert(&$request) {
    if ($this->client && isset($this->client->private_cert)) {
      if (file_exists($this->client->private_cert) && is_file($this->client->private_cert)) {
        return 'file://' . $this->client->private_cert;
      }
    }
    throw new Exception(t('An error occurred while attempting to retrieve the private certificate key for Jira REST API client %client.', array('%client' => $this->client->getLabel())));
  }
}
