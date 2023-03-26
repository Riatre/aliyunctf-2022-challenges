import { createApp } from 'vue'
import './style.css'
import App from './App.vue'

let last_captcha_token = undefined;
window.publishCaptchaToken = (token) => {
    last_captcha_token = token;
};
function consumeCaptchaToken() {
    if (last_captcha_token) {
        let ret = last_captcha_token;
        last_captcha_token = undefined;
        return ret;
    }
    return window.getNVCVal();
}
function setCaptchaState(code) {
    switch (code) {
        case 400:
            getNC().then(function () {
                _nvc_nc.upLang('cn', {
                    _startTEXT: "请按住滑块，拖动到最右边",
                    _yesTEXT: "验证通过",
                    _error300: "哎呀，出错了，点击<a href=\"javascript: __nc.reset()\">刷新</a>再来一次",
                    _errorNetwork: "网络不给力，请<a href=\"javascript:__nc.reset()\">点击刷新</a>",
                })
                _nvc_nc.reset()
            })
            break;
        case 600:
            getSC().then(function () { });
            break;
        case 700:
            getLC();
            break;
        default:
            nvcReset();
            break;
    }
}

const app = createApp(App);
app.provide("consumeCaptchaToken", consumeCaptchaToken);
app.provide("setCaptchaState", setCaptchaState);
app.mount('#app');
