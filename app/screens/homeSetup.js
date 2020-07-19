import React, {Component} from 'react';
import {View, Text, TextInput, Platform, Image, ScrollView} from 'react-native';
import {NetworkInfo} from 'react-native-network-info';
import SmartConfigP from 'react-native-smart-config-p';
import Dialog from 'react-native-dialog';
import {Button} from 'react-native-elements';


const MAX_RETRIES = 5;

export class HomeSetupScreen extends Component {
    constructor(props) {
        super(props);
        this.state = {
            retryCount: 0,
            url: 'http://192.168.1.15',
            wifiPaired: false,
            thermostatName: '',
            numberOfVents: 0,
            ventPaired: false,
            ventName: '',
            newVent: '',
            dialogFoundVent: false,
            dialogAddNewVent: false,
            rooms: [],
            addNewVent: false,
            isError: false,
            errorTitle: '',
            errorMessage: '',
            showWiFiPairedDialogue: false,
        };
    }

    smartConfig = () => {
        // this.checkPairingWasSuccessful();
        // this.createRoom();
        console.log('Begin pairing process');
        console.log('Retry Count: ' + this.state.retryCount);
        if (this.state.retryCount < MAX_RETRIES) {
            console.log('Smart Config Attempt ' + this.state.retryCount + 1);
            NetworkInfo.getBSSID().then(bssid => {
                console.log(bssid);
                SmartConfigP.start({
                    // ssid: 'The Real Promised LAN',
                    // password: 'ice&fire',
                    ssid: 'Robinson',
                    password: '3evi872ba2g490',
                    bssid,
                    count: 1,
                    cast: 'broadcast',
                }).then(data => {
                    if (data) {

                        let timerId = setInterval(() => {
                            this.checkPairingWasSuccessful();
                            if (this.state.wifiPaired == true) {
                                // alert("Wifi is paired: " + this.state.wifiPaired);
                                clearInterval(timerId);
                                clearTimeout(timeout);
                            }
                        }, 2000);
                        let timeout = setTimeout(() => {
                            clearInterval(timerId);
                            alert('Smart Config Timed Out');
                        }, 10000);
                        console.log('WIFI PAIRED' + this.state.wifiPaired);

                    }
                }).catch(err => {
                    console.log('Smart config returned with error:' + err);
                    this.setState({retryCount: this.state.retryCount + 1}, () => this.smartConfig());
                });
            });
        } else {
            alert('Wifi Pairing Failed. Make sure device is in pairing mode and there is internet access. The retry count is: ' + this.state.retryCount);
        }
    };

    checkPairingWasSuccessful = () => {
        console.log('Checking if pairing was successful');
        fetch(this.state.url + '/venti/checkWiFiWasPaired.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: 'empty',
        }).then((response) => response.json()).then((res) => {
            console.log(res);
            if (res.message === 1) {
                console.log('Thermostat paired to WiFi');
                this.setState({wifiPaired: true, showWiFiPairedDialogue: true});
            } else {
                console.log('Thermostat is not yet paired to WiFi');
            }
        }).done(() => {
        });
    };

    nameHome = (text) => {

        fetch(this.state.url + '/venti/updateHomeName.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                name: text,
            }),
        }).then((response) => response.json()).then((res) => {
            if (res.message === true) {
                console.log('Updated Home Name');
            } else {
                console.log('Failed to Update Home Name');
            }
        }).done();
    };

    pairNewVent = () => {
        // this.resetVentCompletedFlag();
        this.getHowManyVentsAlreadyPaired().then(response => {
            if (!isNaN(response.message)) {
                console.log('Number of already paired vents: ' + response.message);
                this.setState({
                    numberOfVents: response.message, addNewVent: false,
                });
                this.updateVentPairingFlag(true);

                let timerId = setInterval(() => {
                    this.checkIfVentWasPaired();
                    if (this.state.ventPaired == true) {
                        clearInterval(timerId);
                        clearTimeout(timeout);
                    }
                }, 2000);
                let timeout = setTimeout(() => {
                    this.updateVentPairingFlag(false);
                    clearInterval(timerId);
                }, 60000);
            } else {
                console.error('Server Error. Could not get number of vents');
            }
        });
    };

    resetVentCompletedFlag = () => {
        fetch(this.state.url + '/venti/resetVentCompletedFlag.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: 'empty',
        }).then((response) => response.json()).then((res) => {
        }).done();
    };


    updateVentPairingFlag = (flag) => {
        fetch(this.state.url + '/venti/updateVentPairingFlag.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                flag: flag,
            }),
        }).then((response) => response.json()).then((res) => {
            if (res.message === true) {
            } else {
                console.log('Failed to set vent pairing flag');
            }
        }).done();
    };

    checkIfVentWasPaired = () => {
        console.log('Checking if vent pairing is successful');
        fetch(this.state.url + '/venti/checkIfVentPaired.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                numVents: this.state.numberOfVents,
            }),
        }).then((response) => response.json()).then((res) => {
            console.log(res);
            if (res.message != '') {
                console.log('New vent with MAC Address: ' + res.message + ' found and paired!');
                this.setState({ventPaired: true, newVent: res.message, dialogFoundVent: true});
            } else {
                console.log('Pairing');
            }
        }).done(() => {
        });
    };

    getHowManyVentsAlreadyPaired = () => {
        return fetch(this.state.url + '/venti/checkHowManyVentsPaired.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: 'empty',
        }).then((response) => response.json())
            .then((responseData) => {
                return responseData;
            });
    };

    createRoom = () => {
        let ventMAC = this.state.newVent;
        let ventName = this.state.ventName;
        console.log('states: ' + this.state.dialogAddNewVent + ',' + !this.state.dialogFoundVent + ',' + this.state.addNewVent + ',' + this.state.ventPaired);
        fetch(this.state.url + '/venti/createRoom.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                name: ventName,
                mac: ventMAC,
            }),
        }).then((response) => response.json()).then((res) => {
            console.log('Create Room Response' + res.message);
            if (res.message != '') {
                console.log('New vent with MAC Address: ' + res.message + ' found and paired!');
                this.setState({
                    ventPaired: false,
                    newVent: res.message,
                    dialogAddNewVent: true,
                    rooms: [...this.state.rooms, ventName],
                    addNewVent: true,
                }, () => {
                    console.log('states: ' + this.state.dialogAddNewVent + ',' + !this.state.dialogFoundVent + ',' + this.state.addNewVent + ',' + this.state.ventPaired);
                });
            } else {
                console.log('Could not create room');
            }
        }).done(() => {
        });
    };

    errorHandler = () => {

    };

    render() {
        return (
            <ScrollView>
                <View style={{alignSelf: 'center', paddingTop: 30}}>
                    <Image style={{height: 175, width: 175}} source={require('../res/images/logo.png')}
                    />
                </View>
                <Dialog.Container visible={this.state.ventPaired && this.state.dialogFoundVent}>
                    <Dialog.Title>Setup Home</Dialog.Title>
                    <Dialog.Description>
                        New vent found with MAC address: {this.state.newVent} found and successfully paired! Give it a
                        name.
                    </Dialog.Description>
                    <Dialog.Input label={'Name your room'} onChangeText={text => {
                        this.setState({ventName: text});
                    }}></Dialog.Input>
                    <Dialog.Button label="Cancel" onPress={() => this.setState({dialogFoundVent: false})}/>
                    <Dialog.Button label="OK" onPress={() => this.setState({dialogFoundVent: false}, () => {
                        this.createRoom();
                    })}/>
                </Dialog.Container>
                <Dialog.Container visible={this.state.showWiFiPairedDialogue}>
                    <Dialog.Title>Thermostat Connection Successful</Dialog.Title>
                    <Dialog.Description>
                        The Venti thermostat was successfully connected to your WiFi. Give it a name and begin pairing
                        your smart vents.
                    </Dialog.Description>
                    <Dialog.Button label="OK" onPress={() => this.setState({showWiFiPairedDialogue: false}, () => {
                    })}/>
                </Dialog.Container>
                <Dialog.Container
                    visible={this.state.dialogAddNewVent && !this.state.dialogFoundVent && this.state.addNewVent}>
                    {/*<Dialog.Container visible={true}>*/}
                    <Dialog.Title>Add another vent?</Dialog.Title>
                    <Dialog.Description>
                        Would you like to add another vent to your home?
                    </Dialog.Description>
                    <Dialog.Button label="No" onPress={() => this.setState({dialogAddNewVent: false}, () => {
                        this.props.navigation.navigate('HomeScreen', {
                            homeName: this.state.thermostatName,
                            rooms: this.state.rooms,
                            isFocused: true,
                        });
                    })}/>
                    <Dialog.Button label="Yes" onPress={() => this.pairNewVent()}/>
                </Dialog.Container>
                <View style={{flexDirection: 'row', paddingBottom: 10, alignSelf: 'center', textAlign: 'center'}}>
                    <View style={{alignSelf: 'center', textAlign: 'center'}}>
                        <Text style={{fontWeight: 'bold', paddingTop: 20, paddingLeft: 15}}>To begin setting up Venti,
                            plug it in and activate the pairing button. Once ready, press Pair Thermostat to
                            begin</Text>
                    </View>
                </View>
                <View style={{flexDirection: 'row', paddingBottom: 10, alignSelf: 'center'}}>
                    <View style={{
                        paddingBottom: 20,
                        paddingTop: 20,
                        alignSelf: 'center',
                        justifyContent: 'center',
                        alignItems: 'center',
                    }}>
                        <Button title="Connect Thermostat" onPress={this.smartConfig} style={{
                            textAlign: 'center',
                            width: 200,
                            alignContent: 'center',
                            justifyContent: 'center',
                            backgroundColor: 'navy',
                            marginTop: 200,
                        }}>
                        </Button>
                    </View>
                </View>
                <View style={{flexDirection: 'row', paddingBottom: 10, alignSelf: 'center'}}>
                    <View>
                        <Text style={{fontWeight: 'bold', paddingTop: 10}}>Give your thermostat a name!</Text>
                    </View>
                </View>
                <View style={{flexDirection: 'row', alignSelf: 'center'}}>
                    <TextInput
                        style={{
                            height: 40,
                            borderColor: 'gray',
                            borderWidth: 1,
                            width: 250,
                            backgroundColor: this.state.wifiPaired ? '#FFF' : '#EDEDED',
                        }}
                        editable={this.state.wifiPaired}
                        onChangeText={text => {
                            this.setState({thermostatName: text}, () => {
                                this.nameHome(text);
                            });
                        }}
                        value={this.state.thermostatName} placeholder="My Place"
                    />
                </View>
                <View style={{flexDirection: 'row', paddingBottom: 10, alignSelf: 'center'}}>
                    <View>
                        <Text style={{fontWeight: 'bold', paddingTop: 80, paddingLeft: 20}}>Once you're ready, press
                            Pair Vent to begin setting up your vents.</Text>
                    </View>
                </View>
                <View style={{flexDirection: 'row', paddingBottom: 10, alignSelf: 'center'}}>
                    <View style={{
                        paddingBottom: 20,
                        paddingTop: 20,
                        alignSelf: 'center',
                        justifyContent: 'center',
                        alignItems: 'center',
                    }}>
                        <Button title="Pair Vent" onPress={this.pairNewVent} disabled={!this.state.wifiPaired} style={{
                            textAlign: 'center',
                            width: 200,
                            alignContent: 'center',
                            justifyContent: 'center',
                            backgroundColor: 'navy',
                            marginTop: 200,
                        }}>
                        </Button>
                    </View>
                </View>

            </ScrollView>
        );
    }
};

export default HomeSetupScreen;
