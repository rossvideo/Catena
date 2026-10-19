Feature: Codegen error handling

  Scenario Outline: Codegen rejects <reason>
    Given the device model "<model>"
    When I run codegen for "<language>"
    Then the command exits with a non-zero status
    And no output files are written

    Examples:
      | model                   | language | reason              |
      | device.invalid.yaml     | cpp      | fails validation    |
      | device.hello_world.yaml | asdf     | unsupported language|
      | device.malformed.yaml   | cpp      | malformed YAML      |
      | param.product.yaml      | cpp      | not a device model  |

  Scenario: A non-existent model path is rejected
    Given a nonexistent device model "device.does_not_exist.yaml"
    When I run codegen for "cpp"
    Then the command exits with a non-zero status
    And no output files are written

  Scenario: Missing mandatory product is rejected but allowed when enforcement is disabled
    Given the device model "device.no_product.yaml"
    When I run codegen for "cpp"
    Then the command exits with a non-zero status
    And no output files are written
    When I run codegen for "cpp" with "--disable-mandatory-enforcement"
    Then the command exits successfully
    And output files are written
