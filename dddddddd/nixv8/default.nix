{ stdenv, lib, fetchgit, fetchFromGitHub
, gn, ninja, python3, glib, pkg-config, icu
, xcbuild, darwin
, fetchpatch
, llvmPackages
, symlinkJoin
}:

# Use update.sh to update all checksums.

let
  version = "11.0.226.16";
  v8Src = fetchgit {
    url = "https://chromium.googlesource.com/v8/v8";
    rev = version;
    sha256 = "0hvabvwxj8xap9s2qm97h58j4kly1396kg15xpxdxarfy670lqnr";
  };

  git_url = "https://chromium.googlesource.com";

  # This data is from the DEPS file in the root of a V8 checkout.
  deps = {
    "base/trace_event/common" = fetchgit {
      url    = "${git_url}/chromium/src/base/trace_event/common.git";
      rev    = "521ac34ebd795939c7e16b37d9d3ddb40e8ed556";
      sha256 = "1zqm9sc98rkr86mzd8mxzcqsqvsyw2ncjha8gv32rxhfzzfz29k4";
    };
    "build" = fetchgit {
      url    = "${git_url}/chromium/src/build.git";
      rev    = "3d4b0c1e773d659da18710fc4984b8195f6d5aea";
      sha256 = "0367rcdfddid3w2gsqqbv1i2yffxvw7m5z5cicihwpqcs56g3zzp";
    };
    "third_party/googletest/src" = fetchgit {
      url    = "${git_url}/external/github.com/google/googletest.git";
      rev    = "af29db7ec28d6df1c7f0f745186884091e602e07";
      sha256 = "0f7g4v435xh830npqnczl851fac19hhmzqmvda2qs3fxrmq6712m";
    };
    "third_party/icu" = fetchgit {
      url    = "${git_url}/chromium/deps/icu.git";
      rev    = "1b7d391f0528fb3a4976b7541b387ee04f915f83";
      sha256 = "0xbqm5xxlyv2rgcz7z4m5si6si67kfqjr9kgb82m09kcarvx0gmf";
    };
    "third_party/zlib" = fetchgit {
      url    = "${git_url}/chromium/src/third_party/zlib.git";
      rev    = "18d27fa10b237fdfcbd8f0c65c19fe009981a3bc";
      sha256 = "15m6hww9diy3zk9wz2w3b8qghf1l7xmlxya2rswpcgswvaysxgk7";
    };
    "third_party/jinja2" = fetchgit {
      url    = "${git_url}/chromium/src/third_party/jinja2.git";
      rev    = "4633bf431193690c3491244f5a0acbe9ac776233";
      sha256 = "1mfzj26fj38szi8xihp1mpl931qn7vgpv4h7yn4h4xyx649ii3jc";
    };
    "third_party/markupsafe" = fetchgit {
      url    = "${git_url}/chromium/src/third_party/markupsafe.git";
      rev    = "13f4e8c9e206567eeb13bf585406ddc574005748";
      sha256 = "1kpqw2ld1n7l99ya6d77d0pcqgsk3waxjasrhqpk29bvk2n5sffy";
    };
  };

  # See `gn_version` in DEPS.
  gnSrc = fetchgit {
    url = "https://gn.googlesource.com/gn";
    rev = "70d6c60823c0233a0f35eccc25b2b640d2980bdc";
    sha256 = "04md36i6l07c1bq8mqghrnbf308j9avmqkwqjqm8gciclnrnlsii";
  };

  myGn = gn.overrideAttrs (oldAttrs: {
    version = "for-v8";
    src = gnSrc;
  });

in

stdenv.mkDerivation rec {
  pname = "v8";
  inherit version;

  doCheck = true;

  patches = [
    ./darwin.patch
  ];

  src = v8Src;

  postUnpack = ''
    ${lib.concatStringsSep "\n" (
      lib.mapAttrsToList (n: v: ''
        mkdir -p $sourceRoot/${n}
        cp -r ${v}/* $sourceRoot/${n}
      '') deps)}
    chmod u+w -R .
  '';

  postPatch = ''
    ${lib.optionalString stdenv.isAarch64 ''
      substituteInPlace build/toolchain/linux/BUILD.gn \
        --replace 'toolprefix = "aarch64-linux-gnu-"' 'toolprefix = ""'
    ''}
    ${lib.optionalString stdenv.isDarwin ''
      substituteInPlace build/config/compiler/compiler.gni \
        --replace 'strip_absolute_paths_from_debug_symbols = true' \
                  'strip_absolute_paths_from_debug_symbols = false'
      substituteInPlace build/config/compiler/BUILD.gn \
        --replace 'current_toolchain == host_toolchain || !use_xcode_clang' \
                  'false'
    ''}
    ${lib.optionalString (stdenv.isDarwin && stdenv.isx86_64) ''
      substituteInPlace build/config/compiler/BUILD.gn \
        --replace "-Wl,-fatal_warnings" ""
    ''}
    touch build/config/gclient_args.gni
    sed '1i#include <utility>' -i src/heap/cppgc/prefinalizer-handler.h # gcc12
  '';

  llvmCcAndBintools = symlinkJoin { name = "llvmCcAndBintools"; paths = [ stdenv.cc llvmPackages.llvm ]; };

  gnFlags = [
    "use_custom_libcxx=false"
    "is_clang=${lib.boolToString stdenv.cc.isClang}"
    "use_sysroot=false"
    # "use_system_icu=true"
    "clang_use_chrome_plugins=false"
    "is_component_build=false"
    "v8_use_external_startup_data=false"
    "v8_monolithic=true"
    "is_debug=true"
    "is_official_build=false"
    "treat_warnings_as_errors=false"
    "v8_enable_i18n_support=true"
    "use_gold=false"
    # ''custom_toolchain="//build/toolchain/linux/unbundle:default"''
    ''host_toolchain="//build/toolchain/linux/unbundle:default"''
    ''v8_snapshot_toolchain="//build/toolchain/linux/unbundle:default"''
  ] ++ lib.optional stdenv.cc.isClang ''clang_base_path="${llvmCcAndBintools}"''
  ++ lib.optional stdenv.isDarwin ''use_lld=false'';

  env.NIX_CFLAGS_COMPILE = "-O2";
  FORCE_MAC_SDK_MIN = stdenv.targetPlatform.sdkVer or "10.12";

  nativeBuildInputs = [
    myGn
    ninja
    pkg-config
    python3
  ] ++ lib.optionals stdenv.isDarwin [
    xcbuild
    llvmPackages.llvm
    python3.pkgs.setuptools
  ];
  buildInputs = [ glib icu ];

  ninjaFlags = [ ":d8" "v8_monolith" ];

  enableParallelBuilding = true;

  installPhase = ''
    install -D d8 $out/bin/d8
    install -D -m644 obj/libv8_monolith.a $out/lib/libv8.a
    install -D -m644 icudtl.dat $out/share/v8/icudtl.dat
    ln -s libv8.a $out/lib/libv8_monolith.a
    cp -r ../../include $out

    mkdir -p $out/lib/pkgconfig
    cat > $out/lib/pkgconfig/v8.pc << EOF
    Name: v8
    Description: V8 JavaScript Engine
    Version: ${version}
    Libs: -L$out/lib -lv8 -pthread
    Cflags: -I$out/include
    EOF
  '';

  meta = with lib; {
    homepage = "https://v8.dev/";
    description = "Google's open source JavaScript engine";
    maintainers = with maintainers; [ cstrahan proglodyte matthewbauer ];
    platforms = platforms.unix;
    license = licenses.bsd3;
  };
}
