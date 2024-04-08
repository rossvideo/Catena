![Alt](images/Catena%20Logo_PMS2191%20&%20White.png)

# Testing Catena using Postman 
This is a quick guide on how to use Postman to run test with Catena. For a more detailed documentation, go to the [learning center](https://learning.postman.com/docs/introduction/overview/) on the Postman website.

## How to set up Postman
1. Follow [this guide](https://learning.postman.com/docs/getting-started/installation/installation-and-updates/) to install Postman on your device.
2. Once installed open Postman and create a new request by selecting "new" then selecting "gRPC". 
3. Go to the "Service definition" tab and select "Import a .proto file" and import Catena's service.proto file from the interface subdirectory. 
4. Select "Enter URL" and add the IP and port that the service is running on.
5. Select the method you want to test.
6. Write the message for your request then click invoke.

![Alt](images/Postman%20guide%20example.png) 

## Enabling Secure Comms

### Server Side Authentication
1. Click the lock icon beside the IP address.

![Alt](images/Postman%20SSL%201.png)

2. Go to the request settings tab and enter the server name that is used for verification.

![Alt](images/Postman%20SSL%202.png)

### Mutual Authentication
(For instructions on how to create authentication certificates see [Security](Security.md))

3. On the request settings tab and turn "Enable server certificate verification" on.
4. Go to Postman settings -> Certificates.
5. Turn on CA certificates and attach the same ca.crt file used by the server.
6. Click add client certificate.
7. Set the host to the same IP address/domain name used  to call the server.
8. attach the client.crt and client.key files. PFX file can be left empty. 
9. If a passphrase was used when generating the certificates then enter it here, otherwise leave passphrase empty.

![Alt](images/Postman%20mTLS.png)


### Call by Call Authorization
1. Go to the authorization tab and set the type to bearer token. 
2. Create a [JSON web token](https://jwt.io/) and paste it into the token field.

![Alt](images/Postman%20Authorization.PNG)