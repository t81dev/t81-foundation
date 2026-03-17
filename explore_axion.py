import os
import re

def search_files(directory):
    matches = []
    for root, _, files in os.walk(directory):
        for file in files:
            if file.endswith(('.md', '.cpp', '.hpp', '.txt')):
                filepath = os.path.join(root, file)
                try:
                    with open(filepath, 'r', encoding='utf-8') as f:
                        content = f.read()
                        if 'Axion' in content:
                            matches.append(filepath)
                except:
                    pass
    return matches

print("\n".join(search_files('.')))
