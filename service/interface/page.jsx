
import { ServicePage } from '../../service.jsx';
import { Panel } from '../../panel.jsx';
import { Button, DropDown, InputField } from '../../components.jsx';
import { setTextToElement } from '../../extraFunctions.js';


export default class extends ServicePage {
    constructor(_service) {
        super({headerConfig: {}}, _service);
        this.directControlPanel = new DirectControlPanel(this);
        this.alarmPanel = new AlarmPanel(this);
        this.programPanel = new ProgramPanel(this);
    }

    renderContent() {
        this.html.self = <div className='PanelBox'>
            {this.moisturePanel.render()}
            {this.waterVolumePanel.render()}
        </div>;

        this.service.send({type: 'getData'});

        return [
            ...super.renderContent(),
            this.html.self,
        ];
    }


   	renderContent() {
		this.html.self = <div className='PanelBox'>
			{this.directControlPanel.render()}
			{this.programPanel.render()}
			{this.alarmPanel.render()}
		</div>;
		
		this.service.send({type: "getPrograms"});
		this.updateContent();
		return [
			...super.renderContent(),
			this.html.self
		]
	}
		

	setLampState(_lampOn) {
		if (!this.html.icon || !this.openState) return;
		this.html.icon.setAttribute('src', "images/lightBolb" + (_lampOn ? "On" : "Off") + ".png");
		this.directControlPanel.setLampState(_lampOn);
	}

	updatePrograms() {
		let options = this.service.programs.map((_program, _index) => {_program.value = _program; _program.value.index = _index; return _program});
		let alarmOptions = [...options, {name: 'No Alarm', value: {index: false, program: [], trigger: ''}}];
		this.alarmPanel.dropDown.setOptions(alarmOptions);
		this.programPanel.dropDown.setOptions(options);
	}

	updateContent() {
		this.setLampState(this.service.state.lampOn);
		this.updatePrograms();
		this.alarmPanel.setAlarmData(this.service.state.alarm);
	}
}






class DirectControlPanel extends Panel {
	#parent;
	constructor(_parent) {
		super();
		this.#parent = _parent;
		this.html.lightBolbIcon = <img className='panelIcon' src='images/lightBolbOn.png'></img>;
		this.html.lampStatus = <div className='text subText'>Lamp On</div>;

		this.toggleButton = new Button({
			text: "Toggle",
			customClass: 'toggleButton',
			onclick: (_e) => {
				this.#parent.service.toggleLight();
				_e.stopPropagation();
			}
		});
	}

	render() {
		let html = super.render();
		html.classList.add('directControlPanel');
		return html;
	}

	
	setLampState(_lampOn) {
		if (!this.html.lampStatus || !this.html.lightBolbIcon) return;
		setTextToElement(this.html.lampStatus, _lampOn ? "Lamp On" : "Lamp Off");
		this.html.lightBolbIcon.setAttribute('src', "images/lightBolb" + (_lampOn ? "On" : "Off") + ".png");
	}

	renderContent() {
		return [
			this.html.lightBolbIcon,
			<div className='text panelTitle'>{this.#parent.service.name}</div>,
			this.html.lampStatus,
			this.toggleButton.render(),
		];
	}
}


class AlarmPanel extends Panel {
	#parent;
	constructor(_parent) {
		super();
		this.#parent = _parent;
		this.html.icon = <img className='panelIcon' src='images/timerIcon.png'></img>;
		this.dropDown = new DropDown({onChange: () => this.#updateAlarm()});
		this.triggerInputField = new InputField({isTimeInput: true, onBlur: () => this.#updateAlarm()});
	}

	render() {
		let html = super.render();
		html.classList.add('alarmPanel');
		return html;
	}

	renderContent() {
		return [
			this.html.icon,
			<div className='text panelTitle centered'>Alarm</div>,
			<div className='inputWrapper'>
				{this.triggerInputField.render()}
				{this.dropDown.render()}
			</div>
		];
	}

	#updateAlarm() {
		if (!this.dropDown.value) return;
		this.#parent.service.send({type: "prepareProgram", data: {
			trigger: this.triggerInputField.getValue(),
			program: this.dropDown.value.program,
			programIndex: this.dropDown.value.index
		}})
	}
	setAlarmData(_alarm) {
		if (!this.triggerInputField.html.self || !_alarm) return;
		this.triggerInputField.html.self.value = _alarm.trigger ? _alarm.trigger : '00:00';
		if (this.dropDown.options.length <= _alarm.programIndex) return;
		
		let programIndex = _alarm.programIndex;
		if (typeof programIndex != 'number') programIndex = this.dropDown.options.length - 1;
		this.dropDown.setValue(this.dropDown.options[programIndex].value);
	}
}

class ProgramPanel extends Panel {
	#parent;
	constructor(_parent) {
		super();
		this.#parent = _parent;

		this.html.icon = <img className='panelIcon' src='images/executeIcon.png'></img>;
		this.dropDown = new DropDown();
		this.runButton = new Button({
			text: "Run",
			boxy: true,
			onclick: (_e) => {
				if (!this.dropDown.value) return;
				this.#parent.service.send({type: "executeGivenProgram", data: this.dropDown.value.program});
				_e.stopPropagation();
			}
		});
	}

	renderContent() {
		return [
			this.html.icon,
			<div className='text panelTitle centered'>Programs</div>,
			<div className='inputWrapper'>
				{this.runButton.render()}
				{this.dropDown.render()}
			</div>
		];
	}

	render() {
		let html = super.render();
		html.classList.add('programPanel');
		return html;
	}
}

