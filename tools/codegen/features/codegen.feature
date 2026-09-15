Feature: Generate source from a device model

  Scenario Outline: Code Generation from <model> in <language> matches the golden output
    Given the device model "<model>"
    When I run codegen for "<language>"
    Then the command exits successfully
    And the generated files match the golden "<language>" output

    Examples:
      | model                            | language |
      | device.hello_world.yaml          | cpp      |
      | device.hello_world.json          | cpp      |
      | device.params.yaml               | cpp      |
      | device.constraints.yaml          | cpp      |
      | device.commands.yaml             | cpp      |
      | device.menus.yaml                | cpp      |
      | device.language_packs.yaml       | cpp      |
      | device.templates.yaml            | cpp      |

  Scenario: Generation is deterministic across runs
    Given the device model "device.hello_world.yaml"
    When I run codegen for "cpp"
    And I run codegen for "cpp" again
    Then both runs produce identical output

  Scenario: Header carries the do-not-edit banner
    Given the device model "device.hello_world.yaml"
    When I run codegen for "cpp"
    Then the generated header contains "This file was auto-generated"
    And the generated header is guarded with "#pragma once"
