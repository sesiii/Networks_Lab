# # import os
# # import subprocess

# # def get_modified_files():
# #     """ Get the list of modified files in the CSES directory """
# #     result = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
# #     modified_files = [line.split()[-1] for line in result.stdout.splitlines() if line.startswith(" M ")]
# #     return modified_files

# # def commit_and_push():
# #     modified_files = get_modified_files()
    
# #     if not modified_files:
# #         print("No modified files to commit.")
# #         return
    
# #     for file in modified_files:
# #         subprocess.run(["git", "add", file])
# #         commit_message = f"Solved: {os.path.basename(file)}"
# #         subprocess.run(["git", "commit", "-m", commit_message])
    
# #     subprocess.run(["git", "push", "origin", "main"])  # Change branch name if needed
# #     print("Changes committed and pushed successfully!")

# # if __name__ == "__main__":
# #     commit_and_push()


# import os
# import subprocess
# import time

# def get_untracked_files():
#     """ Get a list of untracked (new) files """
#     result = subprocess.run(["git", "ls-files", "--others", "--exclude-standard"], capture_output=True, text=True)
#     return result.stdout.splitlines()

# def get_modified_files():
#     """ Get the list of modified files """
#     result = subprocess.run(["git", "status", "--porcelain"], capture_output=True, text=True)
#     return [line.split()[-1] for line in result.stdout.splitlines() if line.startswith(" M ")]

# def commit_and_push():
#     modified_files = get_modified_files()
#     untracked_files = get_untracked_files()
#     all_files = modified_files + untracked_files

#     if not all_files:
#         print("No new or modified files to commit.")
#         return

#     for file in all_files:
#         subprocess.run(["git", "add", file])
#         commit_message = f"Solved: {os.path.basename(file)}"
#         subprocess.run(["git", "commit", "-m", commit_message])

#         # Push immediately after each commit to increase contribution count
#         subprocess.run(["git", "push", "origin", "main"])  # Change branch if necessary
#         time.sleep(2)  # Small delay to mimic manual commits

#     print("All files committed and pushed successfully!")

# if __name__ == "__main__":
#     commit_and_push()


import os
import subprocess
import time

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

        commit_message = f"changes in {os.path.basename(file)}"
        subprocess.run(["git", "commit", "-m", commit_message])

        # Push immediately after each commit to increase contribution count
        subprocess.run(["git", "push", "origin", "master"])  # Change branch if necessary
        time.sleep(2)  # Small delay to mimic manual commits

    print("All changes committed and pushed successfully!")

if __name__ == "__main__":
    commit_and_push()