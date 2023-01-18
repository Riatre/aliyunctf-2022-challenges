import typer

def main(binary: typer.FileBinaryRead):
    content = binary.read()
    assert b"speck" not in content
    assert b"Speck" not in content

if __name__ == "__main__":
    typer.run(main)