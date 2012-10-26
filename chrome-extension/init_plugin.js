var pluginObj = document.getElementById('pluginObj');
setTimeout(function() {
  console.log('Starting DepthJS...');
  if (!DepthJS.init(pluginObj)) {
    console.log("Could not init DepthJS");
  }
}, 1000);