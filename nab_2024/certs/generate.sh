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
echo "Generating Certificate Authority private key..."
openssl genrsa -passout pass:$pwd -out ca.key 4096

# create the cert authority certificate
echo "Generating Certificate Authority certificate..."
openssl req -passin pass:$pwd -new -x509 -days 365 -key ca.key -out ca.crt -subj "/C=US/ST=TX/L=SanAntonio/O=RossVideoLtd/OU=Catena/CN=catenamedia.tv"

# ================================= server_app_s1 ================================= #
# generate the server's app1 private key
echo "Generating server_app_s1 trust chain for URL=vegas1.catenamedia.tv ..."
openssl genrsa -passout pass:$pwd -out server_app_s1.key 4096

# generate a signing request for the server app1
openssl req -passin pass:$pwd -new -key server_app_s1.key -out server_app_s1.csr -subj "/C=US/ST=TX/L=SanAntonio/O=RossVideoLtd/OU=Catena/CN=vegas1.catenamedia.tv"

# create the server's cert for app1
openssl x509 -req -passin pass:$pwd -days 365 -in server_app_s1.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out server_app_s1.crt

# remove password from server's app1 key
openssl rsa -passin pass:$pwd -in server_app_s1.key -out server_app_s1.key

# ================================= server_app_s1 ================================= #

# ================================= server_app_s2 ================================= #
# generate the server's app1 private key
echo "Generating server_app_s2 trust chain for URL=vegas2.catenamedia.tv ..."
openssl genrsa -passout pass:$pwd -out server_app_s2.key 4096

# generate a signing request for the server
openssl req -passin pass:$pwd -new -key server_app_s2.key -out server_app_s2.csr -subj "/C=US/ST=TX/L=SanAntonio/O=RossVideoLtd/OU=Catena/CN=vegas2.catenamedia.tv"

# create the server's cert
openssl x509 -req -passin pass:$pwd -days 365 -in server_app_s2.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out server_app_s2.crt

# remove password from server's key
openssl rsa -passin pass:$pwd -in server_app_s2.key -out server_app_s2.key
# ================================= server_app_s2 ================================= #

# generate the client's private key
openssl genrsa -passout pass:$pwd -out client.key 4096

# generate a signing request for the client
openssl req -passin pass:$pwd -new -key client.key -out client.csr -subj "/C=US/ST=TX/L=SanAntonio/O=RossVideoLtd/OU=Catena/CN=client.catenamedia.tv"

# create the client's cert
openssl x509 -req -passin pass:$pwd -days 365 -in client.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out client.crt

# remove password from client's key
openssl rsa -passin pass:$pwd -in client.key -out client.key

