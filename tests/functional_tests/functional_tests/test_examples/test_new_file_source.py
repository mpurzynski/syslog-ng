from source.testdb.initializer.setup_testcase import SetupTestcase

def test_new_file_source_anti(request):
    syslog_ng_tc = SetupTestcase(testcase_context=request)

    # add global options
    global_options = {
        "stats_level": 3,
        "time_reopen": 1,
        "time_reap": 1,
    }
    syslog_ng_tc.syslog_ng_config_interface.add_global_options(global_options)

    # add 1st file source
    custom_src_file_path = syslog_ng_tc.file_register.get_registered_file_path(prefix="my_src_file")
    src_file_options = {
        "file_path": custom_src_file_path,
    }
    src_stmt_id, src_drv_prop = syslog_ng_tc.syslog_ng_config_interface.create_source(driver_name="file", driver_options=src_file_options, use_mandatory_options=False)

    # add 1st file destination
    custom_dst_file_path = syslog_ng_tc.file_register.get_registered_file_path(prefix="my_dst_file")
    dst_file_options = {
        "file_path": custom_dst_file_path,
    }
    dst_stmt_id, dst_drv_prop = syslog_ng_tc.syslog_ng_config_interface.create_destination(driver_name="file", driver_options=dst_file_options, use_mandatory_options=False)

    # connect source - destination in logpath
    source_statements = [src_stmt_id]
    destination_statements = [dst_stmt_id]
    logpath = syslog_ng_tc.syslog_ng_config_interface.connect_statements_in_logpath(source_statements, destination_statements)

    # generate config
    syslog_ng_tc.syslog_ng_config_interface.generate_config()

    # add 2nd file destination
    custom_dst_file_path2 = syslog_ng_tc.file_register.get_registered_file_path(prefix="my_dst_file2")
    dst_file_options2 = {
        "file_path": custom_dst_file_path2,
    }
    dst_stmt_id2, dst_drv_prop2 = syslog_ng_tc.syslog_ng_config_interface.create_destination(driver_name="file", driver_options=dst_file_options2, use_mandatory_options=False)

    # connect 2nd file destination to logpath
    logpath['destination_statements'].append(dst_stmt_id2)

    # re-generate config
    syslog_ng_tc.syslog_ng_config_interface.generate_config(use_internal_source=False, re_create_config=True)

    syslog_ng_tc.syslog_ng.start()
    syslog_ng_tc.syslog_ng.reload()

    custom_message_parts = {
        "priority": "99",
        "bsd_timestamp": "Dec  1 08:05:04",
        "hostname": "test-machine",
        "program": "test-program",
        "pid": "1111",
        "message": "Test message"
    }
    custom_message_for_sending = syslog_ng_tc.message_interface.create_bsd_message(defined_bsd_message_parts=custom_message_parts)

    # src_drv_prop['writer_class'].write_content(file_path=src_drv_prop['connection_mandatory_options'], content=custom_message_for_sending, driver_name="file")
    src_drv_prop['writer'].send(message="AAAAAAAAAAAAAAAAAAAAAAAAAAAAA")

    message = dst_drv_prop['listener'].receive()
    assert "AAAAAAAAAAAAAAAAAAAAAAAAAAAAA" in message[0]
    syslog_ng_tc.syslog_ng.stop()

#
# def test_new_file_source_anti_shorter(request):
#     syslog_ng_tc = SetupTestcase(testcase_context=request)
#     syslog_ng_tc.syslog_ng_config_interface.add_global_options({"stats_level":3, "time_reopen":3})
#     src_stmt_id, src_drv_prop = syslog_ng_tc.syslog_ng_config_interface.create_source(driver_name="file") # file_path will be random generated
#     dst_stmt_id, dst_drv_prop = syslog_ng_tc.syslog_ng_config_interface.create_destination(driver_name="file") # file_path will be random generated
#     logpath = syslog_ng_tc.syslog_ng_config_interface.connect_statements_in_logpath([src_stmt_id], [dst_stmt_id])
#     syslog_ng_tc.syslog_ng_config_interface.generate_config()
#
#     dst_stmt_id2, dst_drv_prop2 = syslog_ng_tc.syslog_ng_config_interface.create_destination(driver_name="file") # file_path will be random generated
#     logpath['destination_statements'].append(dst_stmt_id2)
#     syslog_ng_tc.syslog_ng_config_interface.generate_config(use_internal_source=False)
#     print("src_file_path: %s" % src_drv_prop['connection_mandatory_options']) # get random generated file path (source)
#     print("dst_file_path: %s" % dst_drv_prop['connection_mandatory_options']) # get random generated file path (dest)
#     print("dst_file_path2: %s" % dst_drv_prop2['connection_mandatory_options']) # get random generated file path (dest2)
