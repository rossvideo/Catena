::: {.image-wrapper style="background-color: black; padding: 5px;"}
![Catena Logo](images/Catena%20Logo_PMS2191%20&%20White.png){style="width: 100%;"}
:::

# ListLanguages
Gets the languages supported by the device model.

### IN
``` proto
uint32 slot = 1; // Uniquely identifies the device at node scope.
```

### OUT
``` proto
/* Language List
 * The list of language tags that the device supports.
 *
 * e.g.
 * [
 *    "en",
 *    "es",
 *    "fr"
 * ] // Support for English, Spanish & French
 *
 * e.g.
 * [] // No multi-lingual support */
message LanguageList {
  repeated string languages = 1;
}
```

<div style="text-align: center">

[Next Page: RefreshToken](RefreshToken.html)

</div>