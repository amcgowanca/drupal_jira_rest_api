<?php
/**
 * @file
 * includes/JiraRestApiClientAuthenticator.inc
 */

/**
 * Base implementation of JiraRestApiClientAuthenticatorInterface.
 */
abstract class JiraRestApiClientAuthenticator implements JiraRestApiClientAuthenticatorInterface {
  /**
   * Factor method for creating new instances by specified client.
   *
   * @access public
   *
   * @param JiraRestApiClient $client
   *   The client instance to create authenticator for.
   *
   * @return JiraRestApiClientAuthenticatorInterface
   *   Returns the authenticator instance. Will return NULL if the client's
   *   defined authenticator type name does not exist as a valid authenticator.
   *
   * @throws Exception
   *   Throws an exception if:
   *     * The authenticator class does not implement the interface
   *       JiraRestApiClientAuthenticatorInterface.
   *     * Inability to instantiate the authenticator class.
   *     * The class does not exist.
   */
  final public static function factory(JiraRestApiClient $client) {
    $authenticators = jira_rest_api_authenticators();
    if (!empty($authenticators[$client->getAuthTypeName()])) {
      $authenticator = $authenticators[$client->getAuthTypeName()];
      if (!empty($authenticator['handler'])) {
        $class_file = $authenticator['file path'] . '/' . $authenticator['file'];
        if (file_exists($class_file)) {
          include_once $class_file;
        }

        $classname = $authenticator['handler'];
        if (class_exists($classname)) {
          $instance = new $classname();
          if (!$instance instanceof JiraRestApiClientAuthenticatorInterface) {
            throw new Exception(t('The class, %classname, does not implement the interface JiraRestApiClientAuthenticatorInterface.', array('%classname' => $classname)));
          }
          if ($instance instanceof JiraRestApiClientAuthenticator) {
            $instance->info = $authenticator;
          }
          $instance->setClient($client);
          return $instance;
        }
        else {
          throw new Exception(t('Unable to instantiate class, %classname, as the class does not exist.', array('%classname' => $classname)));
        }
      }
      else {
        throw new Exception(t('Unable to create an instance of the %label authenticator as no handler class is defined.', array('%label' => $authenticator['label'])));
      }
    }
    return NULL;
  }


  /**
   * @var JiraRestApiClient
   *   The client instance.
   */
  private $client = NULL;

  /**
   * @var array
   *   The authenticator's information.
   *
   * @see hook_jira_rest_api_authenticator().
   */
  private $info = array();

  /**
   * Constructor.
   *
   * @access public
   *
   * @param JiraRestApiClient $client
   *   The client instance for this authenticator.
   */
  public function __construct(JiraRestApiClient $client = NULL) {
    if ($client) {
      $this->setClient($client);
    }
  }

  /**
   * {@inheritdoc}
   */
  final public function setClient(JiraRestApiClient $client) {
    $this->client = $client;
  }

  /**
   * Returns the JiraRestApiClient instance for this authenticator.
   *
   * @access public
   *
   * @return JiraRestApiClient
   *   The client instance.
   */
  final protected function getClient() {
    return $this->client;
  }

  /**
   * Returns authenticator's information.
   *
   * @access public
   *
   * @return array
   *   The information.
   */
  final public function getInfo() {
    return $this->info;
  }

  /**
   * {@inheritdoc}
   */
  final public function getType() {
    return $this->info['name'];
  }

  /**
   * {@inheritdoc}
   */
  final public function getLabel() {
    return $this->info['label'];
  }
}