::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](../images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# AddLanguage
Adds a language pack to a device model. This gRPC requires admin write if authorization is enabled.

### IN
```
/* AddLanguagePayload
 * Used by a client to request that a device adds support for a language pack. */
message AddLanguagePayload {
  uint32 slot = 1;                /* Uniquely identifies the device at node
                                   * scope. */
  LanguagePack language_pack = 2; // The language pack to add.
  string id = 3;                  // e.g. "es" Global Spanish.
}

/* Language Pack
 * A map of string identifiers, e.g. "greeting" to strings which are all
 * in the same language. */
message LanguagePack {
  string name = 1;               // e.g. "Global Spanish"
  map<string, string> words = 2; // e.g. "greeting" -> "Hola"
}
```

### OUT
This gRPC returns nothing.

<div style="text-align: center">

[Next Page: LanguagePackRequest](LanguagePackRequest.html)

</div>