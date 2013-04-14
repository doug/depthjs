function main() {
    var pluginObj = document.getElementById('pluginObj');
    setTimeout(function() {
               console.log('Starting DepthJS...');
               if (!DepthJS.init(pluginObj)) {
                console.log("Could not init DepthJS");
               }
               }, 1000);
}

document.addEventListener('DOMContentLoaded', function () {
                          main();
                          });