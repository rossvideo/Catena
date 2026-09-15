Feature: Codegen error handling

  Scenario: Invalid device model fails validation
    Given the device model "device.invalid.yaml"
    When I run codegen for "cpp"
    Then the command exits with a non-zero status
    And no output files are written

  Scenario: Unsupported language is rejected
    Given the device model "device.hello_world.yaml"
    When I run codegen for "asdf"
    Then the command exits with a non-zero status
    And no output files are written

  Scenario: Malformed YAML is rejected
    Given the device model "device.malformed.yaml"
    When I run codegen for "cpp"
    Then the command exits with a non-zero status
    And no output files are written

  Scenario: A non-existent model path is rejected
    Given a nonexistent device model "device.does_not_exist.yaml"
    When I run codegen for "cpp"
    Then the command exits with a non-zero status
    And no output files are written

  Scenario: A non-device model file is rejected
    Given the device model "param.product.yaml"
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
