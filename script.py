import os
import subprocess
import time
from datetime import datetime

def get_untracked_files():
    """ Get a list of untracked (new) files """
    result = subprocess.run(["git", "ls-files", "--others", "--exclude-standard"], capture_output=True, text=True)
    return result.stdout.splitlines()

def get_modified_files():
    """ Get a list of modified files """
    result = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
    return [line.split()[-1] for line in result.stdout.splitlines() if line.startswith(" M ")]

def get_deleted_files():
    """ Get a list of deleted files """
    result = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
    return [line.split()[-1] for line in result.stdout.splitlines() if line.startswith(" D ")]

def commit_and_push():
    modified_files = get_modified_files()
    untracked_files = get_untracked_files()
    deleted_files = get_deleted_files()
    all_files = modified_files + untracked_files + deleted_files

    if not all_files:
        print("No new, modified, or deleted files to commit.")
        return

    for file in all_files:
        if file in deleted_files:
            subprocess.run(["git", "rm", file])  # Remove deleted files
        else:
            subprocess.run(["git", "add", file])  # Add new/modified files

        current_time = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        commit_message = f"changes in {os.path.basename(file)} on {current_time}"
        subprocess.run(["git", "commit", "-m", commit_message])

        # Push immediately after each commit to increase contribution count
        subprocess.run(["git", "push", "origin", "main"])  # Change branch if necessary
        time.sleep(2)  # Small delay to mimic manual commits

    print("All changes committed and pushed successfully!")

if __name__ == "__main__":
    commit_and_push()