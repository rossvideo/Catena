Feature: Serialize and deserialize device models

  Scenario: ser --version prints the tool version
    When I run ser with "--version"
    Then the command exits successfully
    And the output matches a semantic version

  Scenario: des --version prints the tool version
    When I run des with "--version"
    Then the command exits successfully
    And the output matches a semantic version

  Scenario: ser --quiet suppresses progress logging
    Given the device model "device.hello_world.yaml"
    When I serialize the model without --quiet
    Then the command exits successfully
    And the output is not empty
    When I serialize the model
    Then the command exits successfully
    And nothing is printed to standard output

  Scenario: ser writes a binary artifact
    Given the device model "device.hello_world.yaml"
    When I serialize the model
    Then the command exits successfully
    And a binary artifact is produced

  Scenario: des defaults to YAML output
    Given the device model "device.hello_world.yaml"
    When I serialize the model
    And I deserialize the binary
    Then the command exits successfully
    And the deserialized output is "yaml"

  Scenario: des --json emits JSON output
    Given the device model "device.hello_world.yaml"
    When I serialize the model
    And I deserialize the binary as "json"
    Then the command exits successfully
    And the deserialized output is "json"

  Scenario: des --metadata prints only the metadata
    Given the device model "device.hello_world.yaml"
    When I serialize the model
    And I deserialize the binary showing only metadata
    Then the command exits successfully
    And the metadata is printed

  Scenario: ser rejects a missing device model
    Given a nonexistent device model "device.does_not_exist.yaml"
    When I serialize the model
    Then the command exits with a non-zero status

  Scenario: ser rejects a non-device model file
    Given the device model "param.product.yaml"
    When I serialize the model
    Then the command exits with a non-zero status

  Scenario: des rejects a file that is not a serialized device
    Given the device model "device.hello_world.yaml"
    When I deserialize the model file directly
    Then the command exits with a non-zero status

  Scenario Outline: <model> survives a serialize/deserialize round-trip
    Given the device model "<model>"
    When I serialize the model
    Then the command exits successfully
    And a binary artifact is produced
    When I deserialize the binary
    Then the command exits successfully
    And the deserialized model is a superset of the input model
    And the deserialized model still validates against the schema

    Examples:
      | model                      |
      | device.hello_world.yaml    |
      | device.hello_world.json    |
      | device.params.yaml         |
      | device.constraints.yaml    |
      | device.commands.yaml       |
      | device.menus.yaml          |
      | device.language_packs.yaml |
      | device.templates.yaml      |
