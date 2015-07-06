## Drupal Jira REST API

The Jira REST API module aims to provide a simple programmatic interface for other module developers to leverage for interacting with Atlassian's RESTful API for their Jira product. There are no significant capabilities of this module other then for creating new clients (consumers) and authenticating them with the API.

### Installation

#### Common installation

* Download the module and place it within the `sites/all/modules` directory under the directory named `jira_rest_api`.
* Install the module either via the Administrative interface or by using Drush, `drush en jira_rest_api --yes`.

### Information, usage & examples

#### Clients (consumers)

The term Client is used frequently within this module as a way to identify consumers of the RESTful API exposed by Atlassian for their Jira product. Clients have two types of unique identifiers:

* serial identifier (`{jira_rest_api_clients.cid}` the primary key for each client row)
* a machine name (`{jira_rest_api_clients.name}` a portable identifier).

Clients are also associated with an authenticator which provides a mechanism for authenticating, deauthenticating and signing requests with appropriate authorization information (as well as additional goodies). Clients are _lockable_ meaning that they cannot be deleted via the administrative interfaces providing module developers or vendors to create client instances that persist for the life time of their module.

There are two types of authenticators available within this module: _HTTP Basic Auth_ and _OAuth 1.0_.

#### Authentication: HTTP Basic Auth

The most basic of the two available authenticators, the HTTP Basic Authentication authenticator performs authentication with the client's registered instance URL by sending [basic access information](https://en.wikipedia.org/wiki/Basic_access_authentication) (username and password) via HTTP Headers. This information is sent as part of each request to the client's endpoint URL allowing for authentication calls to occur with the provided username and password for a registered user of the Atlassian Jira instance (assuming they have appropriate permissions to interact with the API).

This authenticator requires the following information:

* **Username**: The username of the user to perform authentication with during each API request.
* **Password**: The password of the user.

It is important to note that the recommended authenticator is the OAuth 1.0 (see below) as this authenticator stores passwords in plain-text within the database.

#### Authentication: OAuth 1.0 (three-legged)

The perferred authenticator of the two, the OAuth 1.0 authenticator performs a three-legged authentication of the client that is persistant for long-term. A client instance (consumer) is required to be configured within the Atlassian Jira instance's administration under "Application Links" screen (typically found at the path `plugins/servlet/applinks/listApplicationLinks` of your Atlassian Jira instance) for this authenticator.

##### Creating a consumer within Jira

1. Login to your Atlassian Jira instance as an administrator and browse to "Application Links" (located under Administration > Discover new applications).
2. Enter the URL of the Drupal installation in which you want to register as a consumer and click "Create new link". _If_ you are prompted with a confirmation screen about the entered URL, confirm the entered URL is correct and continue.
3. Enter the application details on the next screen and click "Continue".
    * Application name: The name of your application.
    * Application type: Select _Generic Application_.
4. Click "Edit" beside the newly created application link and then click "Incoming Authentication". (As this module currently only supports making calls to Atlassian Jira and not receiving them, we will need to only configure the Incoming Authentication portion when registering a consumer with the Jira instance as calls are "incoming" to Jira and not outgoing from Jira).
5. On the "Incoming Authentication" screen, enter the following information:
    * Consumer key: This is a unique key that you will need to specify on when creating a client on the Drupal installation.
    * Consumer name: A unique name allowing for easily identifying the Drupal installation.
    * Public key: Paste the contents of your public key certification that will associate with the private key certificate file that will be used at the Drupal installation.
    * Consumer callback: This is a specialized endpoint URL for the Drupal installation in the following path format: `[base-url]/jira-rest-api/authentication-callback/[jira-client-machine-name]`. The `[base-url]` is the base URL of the Drupal installation and the `[jira-client-machine-name]` is a unique machine name of the client created on the Drupal installation (this allows for easily identifying clients when more than one client may exist on the same Drupal installation).
6. Click "Save".

##### Creating a client within Drupal

1. Login to your Drupal installation as an administration and browse to "Jira REST API" (located under Administration > Configuration > Web Services).
2. Click the "Add a client" link located in the action links section of the client list view.
3. Upon clicking "Add a client", you will be presented with the "Create a new client" form. Enter the following information:
    * Label: This is a human-readable label for this client. _Note: You will notice the "machine name" will be automatically generated as a representation of the label. This is the machine name that will need to be used as the value for the token `[jira-client-machine-name]` in Step 5 when configuring incoming authentication for the consumer registered within Atlassian's Jira._
     * Base Jira URL: This is the base URL to the Jira instance.
     * Authentication method: Select _OAuth 1.0_.
     * Jira application consumer key: This is the unique key value. _Note: This must be identical to the value set in Step 5 when configuring incoming authentication for the consumer registered within Atlassian's Jira as the 'Consumer Key'._
     * Private certificate file: The absolute file path (including file name; typically named with an extension `.pem`) to the private key certificate file used in conjunction to the public key certification file for authenticating.
4. Click "Save".

You are now ready to test the authentication by clicking the "Test authentication" button the client form when editing the newly created client.

**Note:** Within this module's GitHub repository, the example signing keys can be used for testing the application. It is not advised to use these keys within any sort of production environment and only used when developing or building your application.

This is the **recommended** authenticator for performing secure transactions with the Atlassian Jira REST API.

#### Resources

A collection of useful resources when working with the Atlassian Jira REST API.

* [Atlassian Jira REST API Information](https://developer.atlassian.com/jiradev/api-reference/jira-rest-apis)
* [Atlassian Jira REST API Documentation](https://docs.atlassian.com/jira/REST/latest/)

### License

This Drupal module is licensed under the [GNU General Public License](./LICENSE.md) version 2.