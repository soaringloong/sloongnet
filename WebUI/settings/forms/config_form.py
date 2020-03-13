'''
@Author: WCB
@Date: 2019-12-14 10:43:20
@LastEditors  : WCB
@LastEditTime : 2019-12-27 15:06:37
@Description: file content
'''
from django import forms
from protocol import protocol_pb2 as protocol


class GlobalConfigForm(forms.Form):
    #module_type = forms.ChoiceField(choices=protocol.ModuleType.items()[2:],initial=2,widget=forms.Select(attrs={'onchange':'OnModuleTypeChange();'}))
    module_type = forms.ChoiceField(
        choices=[(v, k) for k, v in protocol.ModuleType.items()][2:], initial=2)
    listen_port = forms.IntegerField( initial=8000, max_value=65535, min_value=1000)
    enable_ssl = forms.BooleanField(required=False)
    cert_file_path = forms.CharField(required=False,help_text="config just when ssl is enabled.")
    key_file_path = forms.CharField(required=False,help_text="config just when ssl is enabled.")
    cert_passwd = forms.CharField(required=False,help_text="config just when ssl is enabled.")
    connect_time = forms.IntegerField( initial=5,
        min_value=0, max_value=20, help_text="set 0 to disabled")
    receive_time = forms.IntegerField(initial=5,
        min_value=0, max_value=20, help_text='set 0 to disabled')
    log_path = forms.CharField(initial="/data/log")
    log_level = forms.ChoiceField(
        choices=[(v, k) for k, v in protocol.LogLevel.items()])
    debug_mode = forms.BooleanField()
    MQ_thread_quantity = forms.IntegerField(initial=3,min_value=1, max_value=200)
    process_thread_quantity = forms.IntegerField(initial=3,min_value=1, max_value=200)
    priority_size = forms.IntegerField(initial=3,min_value=0, max_value=10)


class FirewallConfigForm(forms.Form):
    block_time = forms.IntegerField(initial=3,min_value=1, max_value=100)


class ProcessConfigForm(forms.Form):
    lua_context_quantity = forms.IntegerField(initial=10,min_value=1,  max_value=300)
    lua_script_folder = forms.CharField(initial="/app/scripts/")
    lua_entry_file = forms.CharField(initial='main.lua')
    lua_entry_function = forms.CharField(initial='init')
    lua_process_function = forms.CharField(initial='process')
    lua_socket_close_function = forms.CharField(initial='socketclose')


class GatewayConfigForm(forms.Form):
    client_check_time = forms.IntegerField()
    client_check_key = forms.CharField(max_length=65, required=False)
    timeout_check_interval = forms.IntegerField()
    timeout_value = forms.IntegerField()


class DataConfigForm(forms.Form):
    data_receive_port = forms.IntegerField()
    data_receive_time = forms.IntegerField()


class DBConfigForm(forms.Form):
    db_server_address = forms.CharField()
    db_server_port = forms.CharField()
    db_user = forms.CharField()
    db_password = forms.PasswordInput()
    db_database = forms.CharField()


class FormConvert():
    @staticmethod
    def to_protocol(src):
        if isinstance(src, GlobalConfigForm):
            config = protocol.GLOBAL_CONFIG()
            config.Type = int(src.cleaned_data['module_type'])
            config.ListenPort = int(src.cleaned_data['listen_port'])
            config.EnableSSL = bool(src.cleaned_data['enable_ssl'])
            config.CertFilePath = src.cleaned_data['cert_file_path']
            config.KeyFilePath = src.cleaned_data['key_file_path']
            config.CertPasswd = src.cleaned_data['cert_passwd']
            config.ConnectTime = int(src.cleaned_data['connect_time'])
            config.ReceiveTime = int(src.cleaned_data['receive_time'])
            config.LogPath = src.cleaned_data['log_path']
            config.LogLevel = int(src.cleaned_data['log_level'])
            config.DebugMode = bool(src.cleaned_data['debug_mode'])
            config.MQThreadQuantity = int(
                src.cleaned_data['MQ_thread_quantity'])
            config.ProcessThreadQuantity = int(
                src.cleaned_data['process_thread_quantity'])
            config.PrioritySize = int(src.cleaned_data['priority_size'])
            return config
        elif isinstance(src, FirewallConfigForm):
            config = protocol.FIREWALL_CONFIG()
            config.BlockTime = int(src.cleaned_data['block_time'])
            return config
        elif isinstance(src, ProcessConfigForm):
            config = protocol.PROCESS_CONFIG()
            config.LuaContextQuantity = int(
                src.cleaned_data['lua_context_quantity'])
            config.LuaScriptFolder = src.cleaned_data['lua_script_folder']
            config.LuaEntryFile = src.cleaned_data['lua_entry_file']
            config.LuaEntryFunction = src.cleaned_data['lua_entry_function']
            config.LuaProcessFunction = src.cleaned_data['lua_process_function']
            config.LuaSocketCloseFunction = src.cleaned_data['lua_socket_close_function']
            return config
        elif isinstance(src, GatewayConfigForm):
            config = protocol.GATEWAY_CONFIG()
            config.ClientCheckTime = int(
                src.changed_data['client_check_time'])
            config.ClientCheckKey = src.changed_data['client_check_key']
            config.TimeoutCheckInterval = int(
                src.changed_data['timeout_check_interval'])
            config.TimeoutTime = int(src.changed_data['timeout_value'])
            return config
        elif isinstance(src, DataConfigForm):
            config = protocol.DATA_CONFIG()
            config.DataReceivePort = int(
                src.changed_data['data_receive_port'])
            config.DataReceiveTime = int(
                src.changed_data['data_receive_time'])
            return config
        elif isinstance(src, DBConfigForm):
            config = protocol.DB_CONFIG()
            config.ServerAddress = src.changed_data['db_server_address']
            config.ServerPort = src.changed_data['db_server_port']
            config.User = src.changed_data['db_user']
            config.Passwd = src.changed_data['db_password']
            config.Database = src.changed_data['db_database']
            return config