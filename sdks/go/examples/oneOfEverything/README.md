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

The **Authorize** button works one of two ways:

1. Paste an access token. A leading `Bearer ` is stripped. The token is kept for this browser tab and sent on every Catena request, including the live update stream.
2. **Generate demo token** builds an unsigned JWT for the role you pick (the same roles as the Auth slot's table). Use this when signature validation is off.

## Exercise authorization without Keycloak

Signature validation stays on unless you pass `--jwt-insecure-skip-signature-validation` so to run the example without a keyclock/signature validation you must pass in that flag:

```bash
go run . --use-rest --authz --jwt-insecure-skip-signature-validation
```

In the page, open **Authorize**, pick a scope to create a token wiht, and choose **Generate unsigned**. This will automatically generate and apply a token for your current session.

## Keycloak and signature checks

`keycloak/` runs a local realm that signs access tokens with ES256 (the SDK default).

In another terminal in this devcontainer run:
```bash
docker compose -f keycloak/docker-compose.yml up -d
```

Leave Keycloak's hostname at `http://localhost:8180`. That is the address your browser opens, and it is the issuer (`iss`) written into each token. `--jwt-issuer` must be that same URL.

Docker publishes Keycloak on the host, so from inside the devcontainer `localhost:8180` does not reach it. `10.255.255.254` is the host address the devcontainer can use. `KEYCLOAK_INTERNAL_URL` tells the example to download signing keys from there while it still checks that tokens were issued by `localhost`.

Start the example:
``` bash
KEYCLOAK_INTERNAL_URL=http://10.255.255.254:8180 \
  go run . --use-rest --authz --jwt-issuer=http://localhost:8180/realms/catena
```

Currently nothing will work due to not having a valid token. Sign in through the browser to get one for a real user.

Open http://localhost:9080/, choose **Authorize**, pick the scopes, and click **Sign in with Keycloak**.

The browser is sent to Keycloak's login page (authorization code flow with PKCE). Sign in as `commissioner` with the password: `demo`. Keycloak sends the browser back to the demo, and the page exchanges that login code for a signed access token belonging to the account you used. The requested scopes are the ones checked in **Authorize**, plus `openid`.

Optionally you can also generate a signed token and paste it in. The following command can be used in the terminal:
```bash
 curl -s -X POST http://10.255.255.254:8180/realms/catena/protocol/openid-connect/token \
  -d grant_type=password \
  -d client_id=oneofeverything \
  -d username=commissioner \
  -d password=demo \
  -d 'scope=openid st2138:mon st2138:mon:w st2138:op st2138:op:w st2138:cfg st2138:cfg:w st2138:adm st2138:adm:w'
```

Stop Keycloak with `docker compose -f keycloak/docker-compose.yml down`.
