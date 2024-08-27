import { Subscriber, SubscriptionList, DeviceService, ServiceFileManager } from '../../../serviceLib.js';


function CustomSubscriber(_config) {
    Subscriber.call(this, {..._config, handleRequest: handleRequest});
    const This = this;
    async function handleRequest(_message) {
        // Intercept messages
        switch (_message.type)
        {
            case "getPrograms": 
                return This.onEvent({type: "programs", data: await This.service.programManager.getPrograms()});
            case "getAlarmData": 
                return This.onEvent({type: "alarmData", data: await This.service.alarmManager.getAlarm()});
        }

        // Servermodified messages
        switch (_message.type)
        {
            case "prepareProgram": 
                if (!_message.data) return This.onEvent({error: "Data missing", message: _message});
                _message.data.trigger = filterTriggerString(_message.data.trigger);
                if (_message.data.programIndex === false) _message.data.trigger = '';
                
                This.service.send(_message);
                This.service.alarmManager.setAlarm(_message.data);
                This.service.pushCurState();
            break;
        }

        // Default messages
        return This.service.send(_message);
    }

    function filterTriggerString(_str) {
        try {
            let parts = _str.split(':');
            let hours = parseInt(parts[0]);
            let minutes = parseInt(parts[1]);
            
            while (hours < 0) hours += 24;
            while (hours > 23) hours -= 24;
            while (minutes < 0) minutes += 60;
            while (minutes > 59) minutes -= 60;

            return intToTwoCharString(hours) + ":" + intToTwoCharString(minutes);
        } catch (e) {return false;} // Will never happen
    }
    function intToTwoCharString(_int) {
        if (_int > 9) return String(_int);
        return "0" + _int;
    }
}



export default class extends DeviceService {

    programManager = new (function(_service) {
        let fm = new ServiceFileManager({path: "programs.json", defaultValue: []}, _service);
        this.getPrograms = async function() {
            return await fm.getContent();
        }
    })(this);

    alarmManager = new (function(_service) {
        let fm = new ServiceFileManager({path: "alarm.json", defaultValue: {programIndex: 0, trigger: "08:00"}}, _service);
        this.getAlarm = async function() {
            return await fm.getContent();
        }
        this.setAlarm = async function(_data) {
            let data = {programIndex: _data.programIndex, trigger: _data.trigger};
            if (!data.trigger || !_data) data = {};
            _service.curState.alarm = data;
            return await fm.writeContent(data);
        }
    })(this);


    constructor({id, config}) {
        super(arguments[0], CustomSubscriber);
    }


    async setup() {
        this.curState.alarm = await this.alarmManager.getAlarm();
    }

    onMessage(_message) {
        switch (_message.type)
        {
            case "sensorState": this.dataManager.addDataRow(_message.data); break;
        }
        
        this.pushEvent(_message);
    }





    async onDeviceConnect() {
        if (!this.curState.alarm || !this.curState.alarm.trigger) return;
        let program = (await this.programManager.getPrograms())[this.curState.alarm.programIndex];
        if (!program) return;
        let data = {
            trigger: this.curState.alarm.trigger,
            program: program.program
        }
        this.send({type: 'prepareProgram', data: data});
    }
}






