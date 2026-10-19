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

  Scenario: A malformed device model URL is rejected
    # a well-formed descriptor filename is shaped schema.name.filetype (three
    # dot-separated chunks); this one has only two
    Given the device model "hello_world.yaml"
    When I run codegen for "cpp"
    Then the command exits with a non-zero status
    And no output files are written
