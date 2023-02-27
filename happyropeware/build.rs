use embed_manifest::embed_manifest_file;

fn main() {
    embed_manifest_file("happyropeware.exe.manifest")
        .expect("unable to embed manifest file");
    println!("cargo:rerun-if-changed=happyropeware.exe.manifest");
}