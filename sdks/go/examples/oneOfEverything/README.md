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

The **Authorize** button offers three ways to send a bearer token. A leading `Bearer ` is stripped. The token is kept for this browser tab and sent on every Catena request, including the live update stream.

1. Paste an access token and choose **Apply**.
2. **Generate Unsigned Token** builds an unsigned JWT for the scope you pick. Use this when signature validation is off.
3. **Sign in with Keycloak** opens the keycloak realm's login page and applies the signed access token for the account you use. The scope checkboxes do not apply to this token.

## Authorization without Keycloak

Signature validation stays on unless you pass `--jwt-insecure-skip-signature-validation`. To run the example without Keycloak, pass that flag:

```bash
go run . --use-rest --authz --jwt-insecure-skip-signature-validation
```

In the page, open **Authorize**, check the scopes you want, and choose **Generate unsigned**. The token is applied for this browser tab.

## Sign in with Keycloak

Use a Keycloak realm that is already running. Pass its issuer URL, the realm URL Keycloak publishes (for example `--jwt-issuer=https://keycloak.catena.dev2.rossvideo.com/realms/catena`). That URL is the `iss` claim the server expects, and the address the browser opens for login.

The realm needs a public client named `oneofeverything` and a redirect URI of `http://localhost:9080/*`. Tokens should be signed with ES256, which is what the SDK accepts by default. The server loads the signing keys from the issuer's OpenID Connect discovery document (`/.well-known/openid-configuration`).

Start the example:

```bash
go run . --use-rest --authz --jwt-issuer=https://keycloak.catena.dev2.rossvideo.com/realms/catena
```

Open http://localhost:9080/. Requests fail until you have a valid token.

Choose **Authorize**, then **Sign in with Keycloak**. The browser goes to Keycloak's login page. After you sign in, Keycloak sends the browser back to the demo, and the page exchanges the login code for a signed access token. That token is stored for this browser tab and sent on later requests. Its scopes are based on the account you used to sign in within the keycloak.

### Demo Users

The password for every user is `1234`

Users:
| Username | Role |
|---|---|
| apatel | commissioner |
| jrichter | it-admin |
| svoss | journalist |
| vnavarro | producer |
| egrayson | runner |
| ncaldwell | tech-director |
