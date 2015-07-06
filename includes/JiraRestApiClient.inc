<?php
/**
 * @file
 * includes/JiraRestApiClient.inc
 */

/**
 * Represents a single Jira REST API client instance.
 */
class JiraRestApiClient {
  /**
   * Creates a JiraRestApiClient instance from a database record object.
   *
   * @param stdClass $object
   *   The database object to initialize a JiraRestApiClient instance from.
   *
   * @return JiraRestApiClient
   *   The client instance.
   */
  public static function fromObject($object) {
    if (is_string($object->data)) {
      $object->data = @unserialize($object->data);
    }
    return new self($object->cid, $object->name, $object->label, $object->base_url, $object->auth_type_name, $object->data, $object->locked);
  }

  /**
   * The client's identifier.
   *
   * @var int
   */
  private $cid = NULL;

  /**
   * The client's label.
   *
   * @var string
   */
  private $label = NULL;

  /**
   * The client's machine name.
   *
   * @var string
   */
  private $name = NULL;

  /**
   * The client's base URL.
   *
   * @var string
   */
  private $base_url = NULL;

  /**
   * The client's data store. Used primarily for authentication details.
   *
   * @var array
   */
  private $data = array();

  /**
   * A boolean indicating if this client is locked.
   *
   * @var bool
   */
  private $locked = FALSE;

  /**
   * The client's authenticator type name (e.g. http_basic).
   *
   * @var string
   */
  private $auth_type_name = NULL;

  /**
   * @var JiraRestApiClientAuthenticatorInterface
   */
  private $authenticator = NULL;

  /**
   * Creates a new client instance.
   *
   * @access public
   *
   * @param int $cid
   *   The client identifier.
   * @param string $name
   *   The client machine name.
   * @param string $label
   *   The client label.
   * @param string $base_url
   *   The client's base URL for creating HTTP requests.
   * @param string $auth_type_name
   *   The client's authenticator type name.
   * @param array $data
   *   An array of data for this client.
   * @param bool $locked
   *   A boolean indicating if this client is locked or not.
   *
   * @see JiraRestApiClientAuthenticator::factory().
   */
  public function __construct($cid = NULL, $name = NULL, $label = NULL, $base_url = NULL, $auth_type_name = NULL, array $data = array(), $locked = FALSE) {
    $this->cid = $cid;
    $this->name = $name;
    $this->label = $label;
    $this->base_url = $base_url;
    $this->auth_type_name = $auth_type_name;
    $this->data = $data;
    $this->locked = $locked;

    $this->authenticator = JiraRestApiClientAuthenticator::factory($this);
  }

  public function getIdentifier() {
    return $this->cid;
  }

  public function setIdentifier($value) {
    $this->cid = $value;
  }

  public function getLabel() {
    return $this->label;
  }

  public function setLabel($value) {
    $this->label = $value;
  }

  public function getName() {
    return $this->name;
  }

  public function setName($value) {
    $this->name = $value;
  }

  public function getBaseUrl() {
    return $this->base_url;
  }

  public function setBaseUrl($value) {
    $this->base_url = $value;
  }

  public function getData() {
    return $this->data;
  }

  public function setData(array $data = array()) {
    $this->data = $data;
  }

  public function getAuthTypeName() {
    return $this->auth_type_name;
  }

  public function getAuthenticator() {
    return $this->authenticator;
  }

  public function isLocked() {
    return $this->locked;
  }

  public function lock() {
    $this->locked = TRUE;
  }

  public function unlock() {
    $this->locked = FALSE;
  }

  /**
   * Saves this client instance.
   *
   * @access public
   *
   * @see jira_rest_api_client_save().
   */
  public function save() {
    return jira_rest_api_client_save($this);
  }

  /**
   * Deletes this client instance.
   *
   * @access public
   *
   * @see jira_rest_api_client_delete().
   */
  public function delete($deauthenticate = FALSE) {
    return jira_rest_api_client_delete($this, $deauthenticate);
  }

  /**
   * Performs an API request using this client.
   *
   * @access public
   *
   * @see jira_rest_api_request().
   */
  public function request($endpoint, array $parameters = array()) {
    return jira_rest_api_request($this, $endpoint, $parameters);
  }

  /**
   * Returns the client data value for the specified key.
   *
   * @access public
   *
   * @param string $key
   *   The data element key.
   *
   * @return mixed
   *   The data value.
   */
  public function __get($key) {
    return isset($this->data[$key]) ? $this->data[$key] : NULL;
  }

  /**
   * Sets the client data value for the specified key.
   *
   * @access public
   *
   * @param string $key
   *   The data element key.
   *
   * @param mixed $value
   *   The data element value.
   */
  public function __set($key, $value) {
    $this->data[$key] = $value;
  }

  /**
   * Tests if the data element at the specified key exists.
   *
   * @access public
   *
   * @param string $key
   *   The data element key.
   *
   * @return bool
   *   Returns TRUE if the element exists, otherwise FALSE.
   */
  public function __isset($key) {
    return isset($this->data[$key]);
  }

  /**
   * Unsets the client data element for the specified key.
   *
   * @access public
   *
   * @param string $key
   *   The data element key.
   */
  public function __unset($key) {
    if (isset($this->data[$key])) {
      unset($this->data[$key]);
    }
  }

  /**
   * Returns an standard object representation of this client.
   *
   * @access public
   *
   * @return object
   *   An object ready for `drupal_write_record()`.
   */
  public function toObject() {
    return (object) array(
      'cid' => $this->cid,
      'name' => $this->name,
      'label' => $this->label,
      'base_url' => $this->base_url,
      'auth_type_name' => $this->auth_type_name,
      'data' => $this->data,
      'locked' => $this->locked,
    );
  }
}
