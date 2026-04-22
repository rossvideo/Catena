/** @type {import('jest').Config} */
export default {
    collectCoverage: true,
    // Test helpers are not production code; keep coverage focused on cppgen sources.
    coveragePathIgnorePatterns: [
        '<rootDir>/tests/cpp/mocks/mock-device\\.js'
    ],
    coverageReporters: [
        ["lcov", { "projectRoot": "../../" }],
        ["cobertura", { "projectRoot": "../../" }],
        // as we add more files, we can turn on skipFull to reduce output noise
        ["text", { "skipFull": false }]
    ],
    reporters: [
        "default",
        "jest-junit",
    ]
}
