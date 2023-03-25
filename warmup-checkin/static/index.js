import { createApp } from 'https://unpkg.com/vue@3/dist/vue.esm-browser.prod.js'

const RTF = new Intl.RelativeTimeFormat(navigator.language);
function getRelativeTimeString(date) {
    const timeMs = typeof date === "number" ? date : date.getTime();
    const deltaSeconds = Math.round((timeMs - Date.now()) / 1000);
    const cutoffs = [60, 3600, 86400, 86400 * 7, 86400 * 30, 86400 * 365, Infinity];
    const units = ["second", "minute", "hour", "day", "week", "month", "year"];
    const unitIndex = cutoffs.findIndex(cutoff => cutoff > Math.abs(deltaSeconds));
    const divisor = unitIndex ? cutoffs[unitIndex - 1] : 1;
    console.log(deltaSeconds, divisor);
    return RTF.format(Math.floor(deltaSeconds / divisor), units[unitIndex]);
}
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
    return getNVCVal();
}
const App = {
    data() {
        return {
            msg: "",
            error: "",
            thread: [],
            accept_new_reply: true,
            waiting_reply: false,
            solved: false,
            post_time: getRelativeTimeString(Date.parse("2023-04-01 08:00:00 +0800")),
        };
    },
    async mounted() {
        let resp = await fetch("/reply");
        this.refresh(await resp.json());
    },
    // TODO: add tokenizer
    computed: {
        token_length() {
            return this.msg.length;
        },
        may_submit() {
            return this.token_length > 0 && this.token_length <= 140;
        },
    },
    methods: {
        refresh(history) {
            let thread = [];
            for (let msg of history.thread) {
                thread.push({
                    role: msg.role,
                    reply_to: msg.role == "user" ? "organizer" : "player",
                    content: msg.content,
                    when: getRelativeTimeString(new Date(msg.timestamp * 1000)),
                });
            };
            this.thread = thread;
            this.accept_new_reply = history.accept_new_reply;
            this.solved = history.solved;
        },
        async send() {
            this.waiting_reply = true;
            let nvc_val = consumeCaptchaToken();
            let appended = false;
            let appendie = setTimeout(() => {
                this.thread.push({
                    role: "user",
                    content: this.msg,
                    when: getRelativeTimeString(Date.now())
                });
                appended = true;
            }, 500);
            let resp = await fetch("/reply", {
                method: "POST",
                headers: {
                    "Content-Type": "application/json",
                },
                body: JSON.stringify({
                    msg: this.msg,
                    nvc: nvc_val,
                }),
            });
            let j;
            try {
                j = await resp.json();
            } catch {
                j = { error: `challenge is broken: ${await resp.text()}` };
            }
            clearTimeout(appendie);
            if (j.nvc_code) {
                if (appended) this.thread.pop();
                setCaptchaState(j.nvc_code);
                if (j.nvc_code >= 800) {
                    this.error = "blocked: bot detected";
                }
            } else if (j.error) {
                if (appended) this.thread.pop();
                this.error = j.error;
            } else {
                this.refresh(j);
                this.msg = "";
            }
            this.waiting_reply = false;
        },
        async again() {
            let resp = await fetch("/new", { method: "POST" });
            this.refresh(await resp.json());
        },
    },
};
createApp(App).mount('#app');
