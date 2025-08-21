::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Testing Catena using Postman 
This is a quick guide on how to use Postman to run test with Catena. For a more detailed documentation, go to the [learning center](https://learning.postman.com/docs/introduction/overview/) on the Postman website.

## How to set up Postman
Follow [this guide](https://learning.postman.com/docs/getting-started/installation/installation-and-updates/) to install Postman on your device.

### Sending a gRPC using Postman
1. Once installed open Postman and create a new request by selecting "new" then selecting "gRPC". 
2. Go to the "Service definition" tab and select "Import a .proto file" and import Catena's service.proto file from the interface subdirectory. 
3. Select "Enter URL" and add the IP and port that the service is running on.
4. Select the method you want to test.
5. Write the message for your request then click invoke.

![Alt](images/Postman%20guide%20example.png) 

> For more information on the REST API calls, see [gRPC]()

### Sending a REST API call using Postman
1. Once installed open Postman and create a new request by selecting "new" then selecting "HTTP". 
2. Go to the "URL" tab and enter the IP and port of that the service is running on followed by "st2138-api/v1/slot/endpoint-to-call/fqoid"
    1. **Note that the connect and get-populated-slots API calls require the slot to not be defined.*
3. Go to the "Method" dropdown and select the correct method for your API call.
4. If the API call requires the "Detail_Level" or "Language" headers, click on the "Headers" tab and enter them there.
6. If the API call requires a JSON body, click on the "Body" tab and enter it there.
5. Click invoke.

![Alt](images/Postman%20guide%20example%20REST.png) 

> For more information on the REST API calls, see the [OpenAPI docs]()

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

<div style="text-align: center">

[Next Page: Supported gRPC Services](gRPC.html)

</div>