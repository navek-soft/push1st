#!C:\Users\user\Desktop\Projects\NavekSoft\push1st\push1st-py\push1st-env\Scripts\python.exe
# EASY-INSTALL-ENTRY-SCRIPT: 'user-agent==0.1.9','console_scripts','ua'
__requires__ = 'user-agent==0.1.9'
import re
import sys
from pkg_resources import load_entry_point

if __name__ == '__main__':
    sys.argv[0] = re.sub(r'(-script\.pyw?|\.exe)?$', '', sys.argv[0])
    sys.exit(
        load_entry_point('user-agent==0.1.9', 'console_scripts', 'ua')()
    )
