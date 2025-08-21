::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# RevokeAccess
Revoke access for a subject.

### IN
``` proto
/* 
 * Request to revoke access for a subject.
 * If revoke_all is true, all active connections for the subject will be revoked.
 */
message RevokeAccessPayload {
  string subject = 1;  // e.g. user id, device id, etc.
  string reason = 2;   // e.g. avoid timeout, key rotation, etc.
  bool revoke_all = 3; // True to revoke access for all subjects
}
```

### OUT
``` proto
/* 
 * Response to the RevokeAccess request.
 * Lists sub fields of remaining and revoked subjects.
 */
message RevocationResponse {
  repeated string remaining_subjects = 1; // user / system ids still connected
  repeated string revoked_subjects = 2;   // user / system ids that were revoked
}
```

<div style="text-align: center">

[Next Page: Supported gRPC Services](gRPC.html)

</div>