::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](../images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# LanguagePackRequest
Returns a language pack from the device model.

### IN
```
/* LanguagePackRequestPayload
 * Requests a language pack from the device. */
message LanguagePackRequestPayload {
  uint32 slot = 1;     // Uniquely identifies the device at node scope.
  string language = 2; // e.g. "es" Global Spanish.
}
```

### OUT
The requested lanuage pack as a ComponentLanguagePack.
```
/* A language pack. */
message ComponentLanguagePack {
  string language = 1;            /* Language string is the language code that
                                   * identifies the language e.g. en-uk */
  LanguagePack language_pack = 2;
}

/* Language Pack
 * A map of string identifiers, e.g. "greeting" to strings which are all
 * in the same language. */
message LanguagePack {
  string name = 1;               // e.g. "Global Spanish"
  map<string, string> words = 2; // e.g. "greeting" -> "Hola"
}
```

<div style="text-align: center">

[Next Page: ListLanguages](ListLanguages.html)

</div>