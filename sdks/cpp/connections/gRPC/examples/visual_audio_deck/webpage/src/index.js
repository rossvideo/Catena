const { StrictMode } = React;
const { createRoot } = ReactDOM;

const root = createRoot(document.getElementById('root'));
root.render(
  <StrictMode>
    <App />
  </StrictMode>
);
