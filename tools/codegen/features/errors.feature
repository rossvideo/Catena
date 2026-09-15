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
