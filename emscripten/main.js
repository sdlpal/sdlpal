var strSyncingFs = 'Syncing FS...';
var strDone = 'Done.';
var strDeleting = 'Deleting...';
var strNoSave = 'Cannot find saved games to download';
var strNoData = 'Error: Game data not loaded!';
var strInit = 'Initializing...';
var strLoading = 'Loading';
var strDelConfirm = "This will DELETE your game data and saved games stored in browser cache. Type 'YES' to continue.";

var userLang = navigator.language || navigator.userLanguage;
if (userLang === 'zh-CN' || userLang.startsWith('zh-Hans') ) {
    strSyncingFs = '正在同步文件系统...';
    strDone = '完成。';
    strDeleting = '正在删除...';
    strNoSave = '无法找到可下载的游戏存档！';
    strNoData = '错误：游戏数据未上传。请先上传ZIP格式的游戏数据文件。';
    strInit = '正在初始化...';
    strLoading = '正在加载';
    strDelConfirm = '此操作将删除您浏览器缓存中保存的数据文件及存档。请输入 "YES" 继续：';
} else if (userLang === 'zh-TW' || userLang.startsWith('zh-Hant') ) {
    strSyncingFs = '正在同步檔案系統...';
    strDone = '完成。';
    strDeleting = '正在刪除...';
    strNoSave = '無法找到可下載的遊戲記錄！';
    strNoData = '錯誤：遊戲資料檔未上傳。請先上傳ZIP格式的遊戲資料檔。';
    strInit = '正在初始化...';
    strLoading = '正在加載';
    strDelConfirm = '此操作將刪除您瀏覽器緩存中保存的遊戲資料檔及記錄。請輸入 "YES" 繼續：';
}


var statusElement = document.getElementById('status');
var progressElement = document.getElementById('progress');
var spinnerElement = document.getElementById('spinner');

var Module = {
    preRun: [],
    postRun: [],
    print: function(text) {
        console.log(text);
    },
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
	if (text === '' && Module.setStatus.last.text == strSyncingFs) return;
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
    Module.setStatus(strSyncingFs);
    spinnerElement.style.display = 'inline-block';
    FS.syncfs(true, function (err) {
	spinnerElement.style.display = 'none';
        Module.setStatus(strDone);
    });
}

function loadZip() {
    var fileBtn = document.getElementById('btnLoadZip');
    Module.setStatus(strLoading + ' ' + fileBtn.files[0].name + '...');
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
	Module.setStatus(strSyncingFs);
	FS.syncfs(function (err) {
	    Module.setStatus(strDone);
	    spinnerElement.style.display = 'none';
	});
    });
}

function clearData() {
    if (window.prompt(strDelConfirm) === "YES") {
        var doDelete = function(path) {
            Object.keys(FS.lookupPath(path).node.contents).forEach(element => {
                var stat = FS.stat(path + '/' + element);
                if (stat.mode & 0040000) {
                    doDelete(path + '/' + element);
                    FS.rmdir(path + '/' + element);
                } else {
                    FS.unlink(path + '/' + element);
                }
            });
        };
        Module.setStatus(strDeleting);
        spinnerElement.style.display = 'inline-block';
        doDelete('/data');
        Module.setStatus(strSyncingFs);
        FS.syncfs(false, function (err) {
            spinnerElement.style.display = 'none';
            Module.setStatus(strDone);
        });
    }
}

function downloadSaves() {
    var zip = new JSZip();
    var hasData = false;
    Object.keys(FS.lookupPath('/data').node.contents).forEach(element => {
        if (element.endsWith('.rpg')) {
            var array = FS.readFile('/data/' + element);
            zip.file(element, array);
            hasData = true;
        }
    });
    if (!hasData) {
        window.alert(strNoSave);
        return;
    }
    zip.generateAsync({type:"base64"}).then(function (base64) {
        window.location = "data:application/zip;base64," + base64;
    }, function (err) {
        Module.printErr(err);
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
	Module.setStatus(strNoData);
	return;
    }
    document.getElementById('btnLaunch').style = "display:none";
    document.getElementById('btnLoadZip').style = "display:none";
    document.getElementById('btnDeleteData').style = "display:none";
    runGame();
}

Module.setStatus(strInit);
window.onerror = function(event) {
    Module.setStatus('Exception thrown, see JavaScript console');
    spinnerElement.style.display = 'none';
    Module.setStatus = function(text) {
        if (text) Module.printErr('[post-exception status] ' + text);
    };
};
