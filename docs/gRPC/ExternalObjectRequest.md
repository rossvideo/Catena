::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# ExternalObjectRequest
Gets an external object from the service.

### IN
```proto
message ExternalObjectRequestPayload {
  uint32 slot = 1; // Uniquely identifies the device at node scope.
  string oid = 2;  // ID of external object being requested
}
```

### OUT
An ExternalObjectPayload.

```proto
message ExternalObjectPayload {
  bool cachable = 1;       /* If false, do not cache the external object
                            * payload */
  DataPayload payload = 2; 
}

// When streamed, only the payload field will be used after the first message.
message DataPayload {
  enum PayloadEncoding {
    UNCOMPRESSED = 0;
    GZIP = 1;
    DEFLATE = 2;
  }

  map<string, string> metadata = 1;     /* information about the payload to be
                                         * interpreted by the client (e.g.
                                         * mime-type, filename, etc.) */
  bytes digest = 2;                     /* SHA-256 digest of the payload -
                                         * flags the recipient to respond with
                                         * proceed flag set to true/false to
                                         * resume or cancel payload data */
  PayloadEncoding payload_encoding = 3;

  oneof kind {
  	string url = 4;    // URL containing data for the payload
  	bytes payload = 5; // Data for the payload
  }
}
```

<div style="text-align: center">

[Next Page: ParamInfoRequest](ParamInfoRequest.html)

</div>