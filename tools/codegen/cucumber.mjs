const common = {
  import: ['features/support/**/*.js', 'features/steps/**/*.js'],
  paths: ['features/**/*.feature'],
}

export default {
  ...common,
  format: ['progress'],
};

export const ci = {
  ...common,
  format: [
    ['junit', 'junit-cucumber.xml'],
    'pretty'
  ],
};