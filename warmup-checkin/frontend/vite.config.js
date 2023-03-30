import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import wasm from "vite-plugin-wasm"
import topLevelAwait from "vite-plugin-top-level-await";
import VueI18nPlugin from '@intlify/unplugin-vue-i18n/vite';

// https://vitejs.dev/config/
export default defineConfig({
  build: {
    manifest: true,
    rollupOptions: {
      input: "src/main.js",
    },
    modulePreload: {
      polyfill: false,
    },
  },
  plugins: [wasm(), topLevelAwait(), VueI18nPlugin(), vue()],
})
