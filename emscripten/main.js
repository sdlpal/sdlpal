var statusElement = document.getElementById('status');
var progressElement = document.getElementById('progress');
var spinnerElement = document.getElementById('spinner');

var Module = {
    preRun: [],
    postRun: [],
    print: (function(text) {
        console.log(text);
    })(),
    printErr: function(text) {
        console.error(text);
    },
    canvas: (function() {
        var canvas = document.getElementById('canvas');

        // As a default initial behavior, pop up an alert when webgl context is lost. To make your
        // application robust, you may want to override this behavior before shipping!
        // See http://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15.2
        canvas.addEventListener("webglcontextlost", function(e) { alert('WebGL context lost. You will need to reload the page.'); e.preventDefault(); }, false);

        return canvas;
    })(),
    setStatus: function(text) {
        if (!Module.setStatus.last) Module.setStatus.last = { time: Date.now(), text: '' };
        if (text === Module.setStatus.last.text) return;
	if (text === '' && Module.setStatus.last.text == 'Syncing FS...') return;
        var m = text.match(/([^(]+)\((\d+(\.\d+)?)\/(\d+)\)/);
        var now = Date.now();
        if (m && now - Module.setStatus.last.time < 30) return; // if this is a progress update, skip it if too soon
        Module.setStatus.last.time = now;
        Module.setStatus.last.text = text;
        if (m) {
            text = m[1];
            progressElement.value = parseInt(m[2])*100;
            progressElement.max = parseInt(m[4])*100;
            progressElement.hidden = false;
            spinnerElement.hidden = false;
        } else {
            progressElement.value = null;
            progressElement.max = null;
            progressElement.hidden = true;
            if (!text) spinnerElement.style.display = 'none';
        }
        statusElement.innerHTML = text;
    },
    totalDependencies: 0,
    monitorRunDependencies: function(left) {
        this.totalDependencies = Math.max(this.totalDependencies, left);
        Module.setStatus(left ? 'Preparing... (' + (this.totalDependencies-left) + '/' + this.totalDependencies + ')' : 'All downloads complete.');
    },
    onRuntimeInitialized:function() {
        onRuntimeInitialized();
    }
};

function onRuntimeInitialized() {
    try {
	FS.mkdir('/data');
    } catch (e) {
    }
    FS.mount(IDBFS, {}, '/data');
    Module.setStatus('Syncing FS...');
    spinnerElement.style.display = 'inline-block';
    FS.syncfs(true, function (err) {
	spinnerElement.style.display = 'none';
        Module.setStatus('Done.');
    });
}

function loadZip() {
    var fileBtn = document.getElementById('btnLoadZip');
    Module.setStatus('Loading ' + fileBtn.files[0].name + '...');
    spinnerElement.style.display = 'inline-block';

    var fileInput = document.getElementById("btnLoadZip");
    var zip = new JSZip();
    var file = fileInput.files[0];

    zip.loadAsync(file).then(function(z) {
	z.forEach(function(relativePath, zipEntry) {
	    if (zipEntry.dir) {
		var pathArr = relativePath.split('/');
		var currPath = '/data';
		for (var i = 0; i < pathArr.length; i++) {
		    currPath += '/';
		    currPath += pathArr[i];
		    try { 
			FS.mkdir(currPath.toLowerCase(), 0777);
		    } catch (e) {
		    }
		}
	    } else {
		zip.sync(function(){zipEntry.async('uint8array').then(function(arr) {
		    FS.writeFile('/data/' + relativePath.toLowerCase(), arr, {encoding: 'binary'});
		})});
	    }
	});
	Module.setStatus('Syncing FS...');
	FS.syncfs(function (err) {
	    Module.setStatus('Done.');
	    spinnerElement.style.display = 'none';
	});
    });
}

async function runGame() {
    mainFunc = Module.cwrap('EMSCRIPTEN_main', 'number', ['number', 'number'], {async:true});
    mainFunc(0, 0);
}

function launch() {
    var checkFile = false;
    try {
	if (FS.stat('/data/fbp.mkf').size > 0) {
	    checkFile = true;
	}
    } catch (e) {
    }
    if (!checkFile) {
	Module.setStatus('Error: Game data not loaded!');
	return;
    }
    document.getElementById('btnLaunch').style = "display:none";
    document.getElementById('btnLoadZip').style = "display:none";
    runGame();
}

Module.setStatus('Initializing...');
window.onerror = function(event) {
    Module.setStatus('Exception thrown, see JavaScript console');
    spinnerElement.style.display = 'none';
    Module.setStatus = function(text) {
        if (text) Module.printErr('[post-exception status] ' + text);
    };
};
