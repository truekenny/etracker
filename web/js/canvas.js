let canvas = document.getElementById("map");
let canvasWidth = canvas.width;
let canvasHeight = canvas.height;
let ctx = canvas.getContext("2d");
let canvasData = ctx.getImageData(0, 0, canvasWidth, canvasHeight);

let imageWidth = 1440;
let imageHeight = 720;
let imageSrc = "images/world.png";

let image = new Image();
image.src = imageSrc;

function drawImage() {
    ctx.drawImage(image,
        0, 0, imageWidth, imageHeight,
        0, 0, canvasWidth, canvasHeight);
}

image.onload = function () {
    drawImage();
    canvasData = ctx.getImageData(0, 0, canvasWidth, canvasHeight);

    // Три тестовые точки
    [
        {x: 0, y: 0, r: 255, g: 0, b: 0, a: 255},
        {x: 1, y: 1, r: 255, g: 0, b: 0, a: 255},
        {x: 2, y: 2, r: 0, g: 255, b: 0, a: 255},
        {x: 3, y: 3, r: 0, g: 0, b: 255, a: 255}
    ].forEach(function ({x, y, r, g, b, a}) {
        drawPixelXY(x, y, r, g, b, a);
    });

    // Тестовые точки по краям мира
    [
        {lat: 0, lon: 0},
        {lat: 56.03, lon: 158.81}, // Камчатка 56.0302233,158.8116
        {lat: -33.93, lon: 151.21}, // Сидней -33.931105, 151.217572
        {lat: 40.72, lon: -74.13}, // Нью Йорк 40.7189125,-74.1256401
        {lat: -54.73, lon: -63.81}, // Южная Америка -54.7267363,-63.8125051
        {lat: 66.54, lon: -16.20} // Исландия 66.5370265,-16.1965755
    ].forEach(function ({lat, lon}) {
        drawPixelLatLon(lat, lon, 255, 0, 255, 255);
    });
};


/**
 * Занести точку во временный массив данных (картунка)
 */
function drawPixelXY(x, y, r, g, b, a) {
    let index = (Math.round(x) + Math.round(y) * canvasWidth) * 4;

    canvasData.data[index] = r;
    canvasData.data[index + 1] = g;
    canvasData.data[index + 2] = b;
    canvasData.data[index + 3] = a;
}

function drawPixelLatLon(lat, lon, r, g, b, a) {
    let pixelPerDeg = canvasWidth / 360;

    let x = (lon + 180) * pixelPerDeg;
    let y = (90 - lat) * pixelPerDeg;
    drawPixelXY(x, y, r, g, b, a);
}

/**
 * Отобразить накопленные данные
 */
function updateCanvas() {
    ctx.putImageData(canvasData, 0, 0);

    ctx.globalAlpha = 0.01;
    drawImage();
    ctx.globalAlpha = 1;
    canvasData = ctx.getImageData(0, 0, canvasWidth, canvasHeight);

    setTimeout(function () {
        updateCanvas();
    }, 100);
}

/**
 * Поддержка постоянного подключения
 * @param socket
 */
function ping(socket) {
    socket.send('p');

    setTimeout(function () {
        ping(socket);
    }, 1000);
}

let link = "ws://" + location.host + "/api/websocket";
let socket = new WebSocket(link, "chat");
socket.binaryType = 'arraybuffer';

// Соединение открыто
socket.addEventListener('open', function (event) {
    ping(socket);
    updateCanvas();
});

// Наблюдает за сообщениями
socket.addEventListener('message', function (event) {
    let data = new DataView(event.data);
    let lat = data.getFloat32(0, true);
    let lon = data.getFloat32(4, true);
    let udp = data.getUint8(8);

    //             ^ lat 90
    //        US   |   RU
    //             |      JP
    // -180 -------+------> lon 180
    //             |
    //        BR   |   AUS
    //             | -90

    drawPixelLatLon(lat, lon, udp ? 0 : 255, udp ? 255 : 0, 0, 255);
});

// Наблюдает за ошибками
socket.addEventListener('error', function (event) {
    console.log('Websocket Error ', event);
    alert('Websocket Error')
});
