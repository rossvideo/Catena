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

## Enabling Secure Comm
Authentication can be enabled by selecting the lock icon beside the IP addrress

To use call by call authorization go to the authorization tab and set the type to bearer token. Next create a [JSON web token](https://jwt.io/) and paste it into postman.