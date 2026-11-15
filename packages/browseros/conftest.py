import os
import socket

import pytest

if socket.gethostname().lower() == "hqh" and os.environ.get("WINDRISE_QUEUE_RUN") != "1":
    raise pytest.UsageError("主电脑不跑测试，投排队单")