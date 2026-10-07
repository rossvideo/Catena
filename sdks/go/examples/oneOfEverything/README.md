# One of Everything

Reference server for the Catena Go SDK. The same handlers serve REST and gRPC.

Authorization is **off by default** so the web UI runs without an identity provider. Pass `--authz` when you want the server to require a bearer token.

## Run the demo

From this directory:

```bash
go run . --use-rest
```

Open http://localhost:9080/. No token is required.

Add `--use-grpc` to also listen for gRPC. At least one transport must be enabled.

## Authorize in the web UI

The **Authorize** button works like Swagger's:

- Paste an access token. A leading `Bearer ` is stripped. The token is kept for this browser tab and sent on every Catena request, including the live update stream.
- **Generate demo token** builds an unsigned JWT for the role you pick (the same roles as the Auth slot's table). Use this when signature validation is off.

The **Auth** slot (slot 3) shows the caller's scopes and JWT claims while authorization is on. While authorization is off it explains that those details are hidden, because every caller is treated as fully authorized.

## Exercise authorization without Keycloak

Signature validation stays off unless you pass `--jwt-validate-signature`, so a demo token is enough:

```bash
go run . --use-rest --authz
```

In the page, open **Authorize**, pick a scope to create a token wiht, and choose **Generate unsigned**. This will automatically generate and apply a token for your current session.

## Keycloak and signature checks

`keycloak/` runs a local realm that signs access tokens with ES256 (the SDK default).

In another terminal in this devcontainer:
```bash
KEYCLOAK_URL=http://10.255.255.254:8180 \
  docker compose -f keycloak/docker-compose.yml up -d
```

`KEYCLOAK_URL` sets the address Keycloak writes into each token as the issuer (`iss`). The example server checks that issuer and downloads Keycloak's signing keys from it, so `--jwt-issuer` must use the same URL. Docker publishes Keycloak on the host, so from inside the devcontainer `localhost:8180` does not reach it; `10.255.255.254` is the host address the devcontainer can use. Your browser on your own computer still opens Keycloak at `http://localhost:8180`.

Start the example:
``` bash
go run . --use-rest --authz --jwt-validate-signature --jwt-issuer=http://10.255.255.254:8180/realms/catena
```

Currently nothing will work due to not having a valid token. There are two ways the keycloak can be used to create a signed token.

### 1. Manually make a request to the keycloak

```bash
 curl -s -X POST http://10.255.255.254:8180/realms/catena/protocol/openid-connect/token \
  -d grant_type=password \
  -d client_id=oneofeverything \
  -d username=commissioner \
  -d password=demo \
  -d 'scope=openid st2138:mon st2138:mon:w st2138:op st2138:op:w st2138:cfg st2138:cfg:w st2138:adm st2138:adm:w'
```

This will make a request to the keycloak for a token with a read and write scope. The reponse will include a `access_token` entry.

Open **Authorize** and in the text field paste in the received `access_token` and click **Apply**. 

This will apply the signed token and refresh the window.

### 2. Auto generate the token from the keycloak

Open **Authorize**, pick the scopes, and click **Get Keycloak token**.

The example server asks Keycloak for a signed access token for the demo user and the page applies it. The request goes to `{--jwt-issuer}/protocol/openid-connect/token` with the same scopes as **Generate unsigned**, plus `openid`. Keycloak must be reachable from the example process at the issuer address.

Stop Keycloak with `docker compose -f keycloak/docker-compose.yml down`.
