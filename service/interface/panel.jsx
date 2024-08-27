
import { HomePagePanel } from '../../panel.jsx';
import { setTextToElement } from '../../extraFunctions.js';
import { Button } from '../../components.jsx';


export default class extends HomePagePanel {
    constructor(_service) {
        super({
            size: [1, 2],
        }, _service)
    }
    render() {
        let html = super.render();
        html.className += ' CableLamp hasIcon hasButtonBar';
        return html;
    }

    renderContent() {
       	this.html.lampStatus = <div className='text subText'>lamp off</div>;
		let toggleButton = new Button({
			text: "Toggle",
			onclick: (_e) => {
				this.service.toggleLight();
				_e.stopPropagation();
			}
		});

		this.html.lightBolbIcon = <img className='panelIcon' src='images/lightBolbOn.png'></img>;
		
		let preparedProgramIndicator = 	<div className='preparedProgramIndicator'>
											<img className='alarmIcon' src='images/alarmIcon.png'></img>
											<div className='text'></div>
										</div>;
		this.html.preparedProgramIndicator = preparedProgramIndicator;

		this.setLampState(this.service.state.lampOn);
		let onlineIndicator = this.renderOnlineIndicator();
		this.setOnlineState(this.service.state.deviceOnline);
		this.setPreparedProgramIndicator(this.service.state.alarm);

		this.updateContent();
		return [
			this.html.lightBolbIcon,
			<div className='text panelTitle'>{this.service.name}</div>,
			onlineIndicator,
			this.html.lampStatus,
			<div className='bottomBar'>
				{preparedProgramIndicator}
				{toggleButton.render()}
			</div>
		];
    }

    updateData() {
        this.setOnlineState(this.service.state.deviceOnline);
    }

	setLampState(_lampOn) {
		if (!this.html.lampStatus) return;
		setTextToElement(this.html.lampStatus, _lampOn ? "Lamp On" : "Lamp Off");
		this.html.lightBolbIcon.setAttribute('src', "images/lightBolb" + (_lampOn ? "On" : "Off") + ".png");
	}

	setPreparedProgramIndicator(_program) {
		let text = '';
		this.html.preparedProgramIndicator.classList.add("hide");
		if (_program && _program.trigger) 	
		{
			this.html.preparedProgramIndicator.classList.remove("hide");
			text = _program.trigger;
		}
		setTextToElement(this.html.preparedProgramIndicator.children[1], text);
	}

	updateContent() {
		this.service.send({type: "getPreparedProgram"});
	}
}

