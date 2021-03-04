from linkkit import linkkit

lk = linkkit.LinkKit(
    host_name="cn-shanghai",
    product_key="xxxxxxxxxxx",
    device_name="nnnnnnnn",
    device_secret="",
    product_secret="yyyyyyyyyyyyyyyy")
lk.on_device_dynamic_register = on_device_dynamic_register


def on_device_dynamic_register(rc, value, userdata):
    if rc == 0:
        print("dynamic register device success, rc:%d, value:%s" % (rc, value))
    else:
        print("dynamic register device fail,rc:%d, value:%s" % (rc, value))
