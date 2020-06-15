import React, {Component} from 'react';
import {
    SafeAreaView,
    StyleSheet,
    ScrollView,
    View,
    Text,
    StatusBar,
    TouchableOpacity,
    PermissionsAndroid,
    Platform,
    Image,
} from 'react-native';
import {TextInput} from 'react-native';
import Permissions from 'react-native-permissions';
import {NetworkInfo} from 'react-native-network-info';
import SmartConfigP, {Cast} from 'react-native-smart-config-p';
import {Button} from 'react-native-vector-icons/Entypo';

class HomeScreen extends Component {

    constructor(props) {
        super(props);
        // if(Platform.OS == 'android') this.permission();
        this.state = {
            url: 'http://192.168.0.13',
            room: 'home',
            temp: '0',
        };

    }

    permission = async () => {
        try {
            const granted = await PermissionsAndroid.request(
                PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
                {
                    title: 'Cool Photo App Camera Permission',
                    message:
                        'Cool Photo App needs access to your camera ' +
                        'so you can take awesome pictures.',
                    buttonNeutral: 'Ask Me Later',
                    buttonNegative: 'Cancel',
                    buttonPositive: 'OK',
                },
            );
            if (granted === PermissionsAndroid.RESULTS.GRANTED) {
                console.log('You can use the camera');
            } else {
                console.log('Camera permission denied');
            }
        } catch (err) {
            console.warn(err);
        }
    };

    smartConfig = () => {
        NetworkInfo.getBSSID().then(bssid => {
            console.log(bssid);
            SmartConfigP.start({
                ssid: 'The Real Promised LAN',
                password: 'ice&fire',
                bssid,
                count: 1,
                cast: 'broadcast',
            }).then(data => {
                console.log(data);
            }).catch(err => {
                console.log(err);
            });
        });
    };

    setTemp = () => {
        if (!this.state.room) {
            alert('Please enter the room name.');
            return;
        }
        if (!this.state.temp) {
            alert('Please enter the desired temperature.');
            return;
        }
        fetch(this.state.url + '/venti/setTemp.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                temp: this.state.temp,
                room: this.state.room,
            }),
        }).then((response) => response.json()).then((res) => {
            if (res.message === true) {
                alert('Updated Temperature');
            } else {
                alert('Failed to set temperature');
            }
        }).done();
    };
    getTemp = () => {
        console.log(this.state.url);
        fetch(this.state.url + '/venti/getTemp.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
        }).then((response) => response.json()).then((res) => {
            let responseString = '';
            console.log(res);
            responseString += 'The temperatures are:\n\n';
            for (let i = 0; i < res.message.length; i++) {
                responseString += 'Vent: ' + res.message[i].mac + '\n';
                responseString += 'Temp: ' + res.message[i].temperature + '\n\n';
            }
            alert(responseString);
            console.log(res.message);
        }).done();
    };

    render() {
        return (
            <View style={{flex: 1, paddingTop: 20, paddingLeft:3, backgroundColor:'white'}}>
                <View style={{flexDirection: 'row', paddingBottom: 10}}>
                    <Text style={{fontWeight: 'bold', paddingTop: 10}}>Server URL:</Text>
                    <TextInput
                        style={{height: 40, borderColor: 'gray', borderWidth: 1, width: 250, marginLeft: 20}}
                        onChangeText={text => {
                            this.setState({url: text});
                        }}
                        value={this.state.url} placeholder="URL"
                    />
                </View>
                <View style={{flexDirection: 'row', paddingBottom: 10}}>
                    <Text style={{fontWeight: 'bold', paddingTop: 10}}>Room Name:</Text>
                    <TextInput
                        style={{height: 40, borderColor: 'gray', borderWidth: 1, width: 250, marginLeft: 13}}
                        onChangeText={text => {
                            this.setState({room: text});
                        }}
                        value={this.state.room} placeholder="Room"
                    />
                </View>
                <View style={{flexDirection: 'row'}}>
                    <Text style={{fontWeight: 'bold', paddingTop: 10}}>Temperature</Text>
                    <TextInput
                        style={{height: 40, borderColor: 'gray', borderWidth: 1, width: 250, marginLeft: 13}}
                        onChangeText={text => {
                            this.setState({temp: text});
                        }}
                        value={this.state.temp} placeholder="Temperature"
                    />
                </View>
                <View style={{
                    flexDirection: 'column',
                    paddingTop: 100,
                    alignItems: 'center',
                    justifyContent: 'center',
                    alignContent: 'center',
                }}>
                    <View style={{
                        paddingBottom: 20,
                        alignSelf: 'center',
                        justifyContent: 'center',
                        alignItems: 'center',
                    }}>
                        <Button title="test" onPress={this.setTemp} style={{
                            textAlign: 'center',
                            width: 200,
                            alignContent: 'center',
                            justifyContent: 'center',
                        }}>
                            <Text style={{color: 'white', textAlign: 'center'}}>Set Temperature</Text>
                        </Button>
                    </View>
                    <View style={{
                        paddingBottom: 20,
                        alignSelf: 'center',
                        justifyContent: 'center',
                        alignItems: 'center',
                    }}>
                        <Button title="test" onPress={this.getTemp} style={{
                            textAlign: 'center',
                            width: 200,
                            alignContent: 'center',
                            justifyContent: 'center',
                        }}>
                            <Text style={{color: 'white', textAlign: 'center'}}>Get Temperature</Text>
                        </Button>
                    </View>
                    <View style={{
                        paddingBottom: 20,
                        alignSelf: 'center',
                        justifyContent: 'center',
                        alignItems: 'center',
                    }}>
                        <Button title="test" onPress={this.smartConfig} style={{
                            textAlign: 'center',
                            width: 200,
                            alignContent: 'center',
                            justifyContent: 'center',
                            backgroundColor: 'navy',
                        }}>
                            <Text style={{color: 'white', textAlign: 'center'}}>Pair Thermostat</Text>
                        </Button>
                    </View>
                    <View style={{paddingTop: 50}}>
                        <Image style={{height:100,width:100}}  source={require('../res/images/logo.png')}
                        />
                    </View>
                </View>
                {/*<Button title="test" onPress={()=>SmartConfigP.stop()} />*/}
            </View>
        );
    }
}


const styles = StyleSheet.create({
    scrollView: {},
    engine: {
        position: 'absolute',
        right: 0,
    },
    body: {},
    sectionContainer: {
        marginTop: 32,
        paddingHorizontal: 24,
    },
    sectionTitle: {
        fontSize: 24,
        fontWeight: '600',
    },
    sectionDescription: {
        marginTop: 8,
        fontSize: 18,
        fontWeight: '400',
    },
    highlight: {
        fontWeight: '700',
    },
    footer: {
        fontSize: 12,
        fontWeight: '600',
        padding: 4,
        paddingRight: 12,
        textAlign: 'right',
    },
});


export default HomeScreen;
