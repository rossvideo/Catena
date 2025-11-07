/** @type {import('jest').Config} */
export default {
    preset: null,
    testEnvironment: 'node',
    transform: {},
    collectCoverage: false,
    coverageReporters: [
        ["lcov", { "projectRoot": "../../" }],
        ["cobertura", { "projectRoot": "../../" }],
        // as we add more files, we can turn on skipFull to reduce output noise
        ["text", { "skipFull": false }]
    ],
    reporters: [
        "summary",
        "jest-junit",
        // ["github-actions", {silent: false}],
    ]
}
