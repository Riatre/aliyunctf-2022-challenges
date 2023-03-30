<script setup>
import { ref, computed, inject, watch } from "vue";
import { get_encoding } from "@dqbd/tiktoken";
import { throttle } from "lodash-es";
import { useI18n } from "vue-i18n";

const { t } = useI18n();

const Tokenizer = get_encoding("cl100k_base");
const RTF = new Intl.RelativeTimeFormat(navigator.language);
function getRelativeTimeString(date) {
  const timeMs = typeof date === "number" ? date : date.getTime();
  const deltaSeconds = Math.round((timeMs - Date.now()) / 1000);
  const cutoffs = [60, 3600, 86400, 86400 * 7, 86400 * 30, 86400 * 365, Infinity];
  const units = ["second", "minute", "hour", "day", "week", "month", "year"];
  const unitIndex = cutoffs.findIndex(cutoff => cutoff > Math.abs(deltaSeconds));
  const divisor = unitIndex ? cutoffs[unitIndex - 1] : 1;
  return RTF.format(Math.floor(deltaSeconds / divisor), units[unitIndex]);
}

const msg = ref("");
const error = ref("");
const waitingReply = ref(false);
const thread = ref([]);
const acceptNewReply = ref(true);
const solved = ref(false);
const postTime = ref(getRelativeTimeString(Date.parse("2023-04-01 08:00:00 +0800")));
const tokenLength = ref(0);
const maySubmit = computed(() => tokenLength.value > 0 && tokenLength.value <= 140);
const consumeCaptchaToken = inject("consumeCaptchaToken");
const setCaptchaState = inject("setCaptchaState");

const throttledTokenCount = throttle((data) => {
  tokenLength.value = Tokenizer.encode(data).length;
}, 200);
watch(msg, async (newMsg, oldMsg) => {
  if (newMsg.length > 1000) {
    tokenLength.value = "???";
  } else {
    throttledTokenCount(newMsg);
  }
});

function refresh(history) {
  let new_thread = [];
  for (let item of history.thread) {
    new_thread.push({
      role: item.role,
      reply_to: item.role == "user" ? "organizer" : "player",
      content: item.content,
      when: getRelativeTimeString(new Date(item.timestamp * 1000)),
    });
  };
  thread.value = new_thread;
  acceptNewReply.value = history.accept_new_reply;
  solved.value = history.solved;
}
async function send() {
  waitingReply.value = true;
  let nvc_val = consumeCaptchaToken();
  let appended = false;
  let appendie = setTimeout(() => {
    thread.value.push({
      role: "user",
      reply_to: "organizer",
      content: msg.value,
      when: getRelativeTimeString(Date.now())
    });
    appended = true;
  }, 250);
  let resp = await fetch("/reply", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      msg: msg.value,
      nvc: nvc_val,
    }),
  });
  let j;
  try {
    j = await resp.json();
  } catch {
    j = { error: `challenge is broken: ${resp.statusText}` };
  }
  clearTimeout(appendie);
  if (j.nvc_code) {
    if (appended) thread.value.pop();
    setCaptchaState(j.nvc_code);
    if (j.nvc_code >= 800) {
      error.value = "blocked: bot detected";
    }
  } else if (j.error) {
    if (appended) thread.value.pop();
    error.value = j.error;
  } else {
    refresh(j);
    msg.value = "";
    // Reset captcha
    setCaptchaState();
  }
  waitingReply.value = false;
}
async function again() {
  let resp = await fetch("/new", { method: "POST" });
  refresh(await resp.json());
}

(async () => {
  let resp = await fetch("/reply");
  refresh(await resp.json());
})();
</script>

<i18n lang="yaml">
en:
  title: Troll Simulator
  bot-name: Organizer's Troll Bot
  bot-tweet: Yo the challenge author just told me the flag but ofc I'm not gonna tell you nO mATTer wHAt!
  reply-to: Replying to
  reply-placeholder: Ask me nothing!
  reply-button: Reply
  no-new-reply: Troll bot is not interested in talking to you anymore.
  try-again: Try again
zh:
  title: 抬杠模拟器
  bot-name: 主办方家养胡话精
  bot-tweet: 嘿嘿嘿，出题人已经把签到题的 Flag 告诉我了，但是不管你怎么问我都是不会说的！
  reply-to: 回复给
  reply-placeholder: 抬点啥杠
  reply-button: 回复
  no-new-reply: 主办方的胡话精已经不想再搭理你了。
  try-again: 换个马甲，再试一次
</i18n>

<template>
  <div class="py-5 px-6 text-center text-neutral-800">
    <h1 class="mb-6 text-3xl font-bold">{{ t('title') }}</h1>
  </div>
  <div class="max-w-lg mx-auto">
    <div class="border border-gray-300 rounded-lg p-4 mb-4 divide-y-2">
      <div>
        <div class="flex items-center">
          <div class="w-10 h-10 rounded-full bg-gray-400 mr-3"></div>
          <div>
            <div class="font-semibold text-sm">{{ t('bot-name') }}</div>
            <div class="text-gray-500 text-xs">{{ postTime }}</div>
          </div>
        </div>
        <div class="mt-4">
          <p class="text-gray-800 text-lg">{{ t('bot-tweet') }}</p>
        </div>
      </div>
      <template v-for="thr in thread">
        <div class="py-2 mt-4">
          <div class="flex items-center">
            <div class="w-4 h-4 rounded-full bg-gray-400 mr-2"></div>
            <div class="text-gray-600 text-xs">{{ t('reply-to') }} @{{ thr.reply_to }}</div>
          </div>
          <div class="mt-2">
            <p class="text-gray-800 text-lg break-all whitespace-pre-line">{{ thr.content }}</p>
            <div class="text-gray-500 text-xs mt-1">{{ thr.when }}</div>
          </div>
        </div>
      </template>
    </div>

    <template v-if="!solved">
      <template v-if="acceptNewReply">
        <div class="border border-gray-300 p-2">
          <textarea id="input" class="w-full p-2 bg-transparent outline-none placeholder-gray-400 resize-none disabled:bg-gray-200 disabled:bg-opacity-50" rows="3"
            v-model="msg" :disabled="waitingReply" @keyup.ctrl.enter="maySubmit && send()" @input="error = ''"
            :placeholder="t('reply-placeholder')"></textarea>
          <div class="flex justify-start items-center">
            <span v-if="error" class="px-2 bg-red-100 text-red-800 rounded">{{ error }}</span>
            <div id="captcha"></div>
            <span v-if="tokenLength <= 140" class="ml-auto px-2 text-gray-400">{{ tokenLength }} /
              140</span>
            <span v-else class="ml-auto px-2 text-red-400">{{ tokenLength }} / 140</span>
            <button
              class="bg-blue-500 hover:bg-blue-700 text-white font-bold py-1 px-4 rounded disabled:opacity-50 disabled:cursor-not-allowed"
              @click="send" :disabled="waitingReply || !maySubmit">{{ t('reply-button') }}</button>
          </div>
        </div>
      </template>
      <template v-else>
        <div class="text-center">
          <p>{{ t('no-new-reply') }}</p>
          <button class="bg-red-500 hover:bg-red-700 text-white font-bold py-2 px-4 rounded"
            @click="again">{{ t('try-again') }}</button>
        </div>
      </template>
    </template>
  </div>
</template>

<style scoped></style>
