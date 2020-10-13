let canvas = document.getElementById("map");
let canvasWidth = canvas.width;
let canvasHeight = canvas.height;
let ctx = canvas.getContext("2d");
let canvasData = ctx.getImageData(0, 0, canvasWidth, canvasHeight);

let imageWidth = 1440;
let imageHeight = 695;

let canvasOffsetX = 0;
let canvasOffsetY = 14; // Math.round((imageWidth / 2 - imageHeight) * canvasWidth / imageWidth);

let latK = 1.035;

let image = new Image();
image.src = "images/world.png";

function drawImage() {
    ctx.drawImage(image,
        0, 0, imageWidth, imageHeight,
        canvasOffsetX, canvasOffsetY, canvasWidth, canvasHeight);
}

image.onload = function () {
    drawImage();
    canvasData = ctx.getImageData(0, 0, canvasWidth, canvasHeight);

    // Три тестовые точки
    [
        {x: 1, y: 56, r: 255, g: 0, b: 0, a: 255},
        {x: 2, y: 57, r: 0, g: 255, b: 0, a: 255},
        {x: 3, y: 58, r: 0, g: 0, b: 255, a: 255}
    ].forEach(function ({x, y, r, g, b, a}) {
        drawPixel(x, y, r, g, b, a);
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
        let x = (lon + 180) * 4;
        let y = (90 - lat * latK) * 4;
        drawPixel(x, y, 255, 0, 255, 255);
    });
};


/**
 * Занести точку во временный массив данных (картунка)
 */
function drawPixel(x, y, r, g, b, a) {
    let index = (Math.round(x) + Math.round(y) * canvasWidth) * Math.round(canvasWidth / 360); // 360 - кол-во градусов в полном угле

    canvasData.data[index] = r;
    canvasData.data[index + 1] = g;
    canvasData.data[index + 2] = b;
    canvasData.data[index + 3] = a;
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

let link = "ws://" + location.host + "/websocket";
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

    drawPixel((lon + 180) * 4, 4 * (90 - lat * latK), udp ? 0 : 255, udp ? 255 : 0, 0, 255);
});

// Наблюдает за ошибками
socket.addEventListener('error', function (event) {
    console.log('Websocket Error ', event);
    alert('Websocket Error')
});
