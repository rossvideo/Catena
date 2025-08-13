::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# RefreshToken
Refreshes a client's access token with the one in the RPC metadata.

### IN
``` proto
/* 
 * Request to refresh the access token with the one in the RPC metadata
 * and extend the expiry time of all active connections to the exp 
 * claim in the new token.
 */
message RefreshTokenPayload {
  string reason = 1; // e.g. avoid timeout, key rotation, etc.
}
```

### OUT
``` proto
/* 
 * Response to the RefreshToken request.
 * If the client has any active streaming connections, the connected field
 * will be true and the exp field will be the time that the new token will expire.
 */
message ConnectionStatus {
  bool connected = 1; // True if client has any active streaming connections
  uint64 exp = 2;     // Time that new token will expire
}
```

<div style="text-align: center">

[Next Page: RevokeAccess](RevokeAccess.html)

</div>