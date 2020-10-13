// Прокрутка на 25%
let visibleHeight = window.innerHeight;
let fullHeight = document.body.clientHeight;

if (fullHeight > visibleHeight) {
    window.scroll(0, Math.round((fullHeight - visibleHeight) / 4));
}
