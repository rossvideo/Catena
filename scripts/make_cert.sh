#! /bin/bash

dirpath=$(dirname "$0")
certs="$dirpath/../certs"
mkdir -p "$certs"

openssl req -x509 -out "$certs/localhost.crt" -keyout "$certs/localhost.key" \
  -newkey rsa:2048 -nodes -sha256 \
  -subj '/CN=localhost' -extensions EXT -config <( \
   printf "[dn]\nCN=localhost\n[req]\ndistinguished_name = dn\n[EXT]\nsubjectAltName=DNS:localhost\nkeyUsage=digitalSignature\nextendedKeyUsage=serverAuth")

echo "RUN with these flags: --secure_comms tls --certs certs/ --cert_file localhost.crt --key_file localhost.key"
