::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# Secure Communications

The gRPC repo includes a command line utility called `grpc_cli` which can be used to exercise the unary components of a service. These are the message pairs that return single objects, not a stream of them. To exercise streams, Postman is a good tool however, its support for security is not complete (mutual authentication isn't supported) so `grcp_cli` is the best way to test and demonstrate Catena's secure comms.

In addition to exercising some of the `Catena Service`'s calls (`GetValue` and `SetValue`) it can also be used with three levels of secure communications:

- no security
- server side SSL
- mutual SSL

The C++ SDK includes an example program, `full_service.cpp` which implements these 3 levels of secure communications.

## With no security

Start `full_service` with no options:

Example output:

```sh
 ./full_service                   
Server listening on 0.0.0.0:5255 with secure comms disabled
```

In another shell, run `grpc_cli`.

```sh
% ./grpc_cli  --json_input call localhost:5255 GetValue "{'oid': '/hello'}"
connecting to localhost:5255
float32_value: 12.3
Rpc succeeded with OK status
```

## Server Side Authentication

Server side authentication requires a private key and certificate authority-signed certificate for the server. For test and development purposes, these can be created locally using `openssl`.

Here is a script that creates a local root of trust and uses it to create and sign certificates for both the client and server ends of the connection. For now, we'll just need the server's key and cert.

```sh
#!/bin/bash

# If you care about keeping the passphrase secret, be sure to suppress 
# invocations of this script going into your history
# depending on your shell settings, this can be achieved as easily
# as putting starting the command line with a leading space


# test whether a passphrase was specified on the command line
if [ $# -eq 0 ]
  then
    echo "no passphrase passed as parameter, using \"wibble\""
    pwd='wibble'
  else
    pwd=$1
fi

# generate the cert authority private key
openssl genrsa -passout pass:$pwd -out ca.key 4096

# create the cert authority certificate
openssl req -passin pass:$pwd -new -x509 -days 365 -key ca.key -out ca.crt -subj "/C=US/ST=TX/L=SanAntonio/O=RossVideoLtd/OU=Catena/CN=catenamedia.tv"

# generate the server's private key
openssl genrsa -passout pass:$pwd -out server.key 4096

# generate a signing request for the server
openssl req -passin pass:$pwd -new -key server.key -out server.csr -subj "/C=US/ST=TX/L=SanAntonio/O=RossVideoLtd/OU=Catena/CN=device.catenamedia.tv"

# create the server's cert
openssl x509 -req -passin pass:$pwd -days 365 -in server.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out server.crt

# remove password from server's key
openssl rsa -passin pass:$pwd -in server.key -out server.key

# generate the client's private key
openssl genrsa -passout pass:$pwd -out client.key 4096

# generate a signing request for the client
openssl req -passin pass:$pwd -new -key client.key -out client.csr -subj "/C=US/ST=TX/L=SanAntonio/O=RossVideoLtd/OU=Catena/CN=client.catenamedia.tv"

# create the client's cert
openssl x509 -req -passin pass:$pwd -days 365 -in client.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out client.crt

# remove password from client's key
openssl rsa -passin pass:$pwd -in client.key -out client.key
```

After running this, you should have these files:

- ca.key
- ca.crt
- server.key
- server.crt
- client.key
- client.crt

These are hard-coded into `full_service.cpp` but the path to them is passed as a command line option. On my machine, I created a folder called `test_certs` under my home folder. This location is used in all the examples that follow. Be sure to change the examples if you use a different location.

```sh
% ./full_service --certs ${HOME}/test_certs --secure_comms ssl
Server listening on 0.0.0.0:5255 with secure comms enabled
```

Now we need to invoke `grpc_cli` with instructions to authenticate the server:

```sh
# tell gRPC where to find the certificate authority
% export GRPC_DEFAULT_SSL_ROOTS_FILE_PATH=/path/to/ca.crt

% ./grpc_cli --ssl_target device.catenamedia.tv  --channel_creds_type ssl  --json_input call localhost:5255 GetValue "{'oid': '/hello'}"
connecting to localhost:5255
float32_value: 12.3
Rpc succeeded with OK status
jnaylor@L-REMJNAYL1-MAC build
```

Note that, trying to use the insecure invocation of `grpc_cli` as in our first example will cause error messages to be logged by both the client `ServerReflectionInfo rpc failed. Error code: 14, ...` and the server `OPENSSL_internal:WRONG_VERSION_NUMBER`.

## Mutual Authentication

Start `full_service` with the `--mutual_authc` flag asserted.

```sh
% ./full_service --certs ${HOME}/test_certs --secure_comms ssl --mutual_authc
Server listening on 0.0.0.0:5255 with secure comms enabled
```

Pass the client certificate information to `grpc_cli`.

```sh
% ./grpc_cli --ssl_target device.catenamedia.tv  --channel_creds_type ssl --ssl_client_cert ~/test_certs/client.crt --ssl_client_key ~/test_certs/client.key --json_input call localhost:5255 GetValue "{'oid': '/hello'}"
connecting to localhost:5255
float32_value: 12.3
Rpc succeeded with OK status
```

Note that calling `grpc_cli` without specifying the location of the client key and certificate will cause errors to be logged both at the client `ServerReflectionInfo rpc failed. Error code: 14, ...` and the server `OPENSSL_internal:PEER_DID_NOT_RETURN_A_CERTIFICATE.`.

## Call by Call Authorization

This is acheived by attaching a JSON Web Token `JWT` formatted `Access Token` to each access to the gRPC service. The device should validate the token and its contents before providing permitting the request to proceed.

At time of writing, all the device does is check whether a token is attached to requests. It will deny requests with no token, but permit all that do to proceed. Clearly, more work is needed here!

Also, the token itself should be obtained from a correctly configured authorization server. At time of writing, [jwt.io](https://jwt.io) can be used to create properly formatted tokens that DO NOT contain the correct claims.

Start `full_service` with the `--authz` flag asserted

```sh
% ./common/examples/full_service/full_service --secure_comms ssl --mutual_authc --authz
Server listening on 0.0.0.0:5255 with secure comms enabled
```

Pass a `JWT` to `grpc_cli`:

```sh
% ./grpc_cli --ssl_target device.catenamedia.tv  --channel_creds_type ssl --ssl_client_cert ${HOME}/test_certs/client.crt --ssl_client_key ${HOME}/test_certs/client.key --call_creds access_token=eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJzdWIiOiIxMjM0NTY3ODkwIiwibmFtZSI6IkpvaG4gRG9lIiwiaWF0IjoxNTE2MjM5MDIyLCJzY29wZXMiOiJtb25pdG9yOnJvIG9wZXJhdGU6cncgY29uZmlnOnJvIn0.U76pZPakfhYOhVLPE-uyyDKmTL7f5x59b7Oranolx9c  --json_input call localhost:5255 GetValue "{'oid':'/hello'}"
connecting to localhost:5255
float32_value: 12.3
Rpc succeeded with OK status
```

<div style="text-align: center">

[The End. For Now](index.html)

</div>