Feature: Codegen command-line interface

  Scenario: --version prints the tool version
    When I run codegen with "--version"
    Then the command exits successfully
    And the output matches a semantic version

  Scenario: --quiet suppresses progress logging
    Given the device model "device.hello_world.yaml"
    When I run codegen for "cpp" without --quiet
    Then the command exits successfully
    And the output is not empty
    When I run codegen for "cpp"
    Then the command exits successfully
    And nothing is printed to standard output
