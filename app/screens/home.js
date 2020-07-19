import React, {Component} from 'react';
import {
    SafeAreaView,
    StyleSheet,
    ScrollView,
    FlatList,
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
import Icon from 'react-native-vector-icons/MaterialCommunityIcons';
import Slider from '@react-native-community/slider';
import {Card, ListItem, Button} from 'react-native-elements';
import Dialog from 'react-native-dialog';
import FAB from 'react-native-fab'
// import CircleSlider from '../utilities/circle-slider';

//Add Scheduling
//Room Settings and control
//Get location
//get wifi information

class HomeScreen extends Component {

    constructor(props) {
        super(props);
        if (Platform.OS == 'android') {
            this.permission();
        }
        this.state = {
            url: 'http://192.168.1.15',
            room: 'home',
            temp: '0',
            city: 'Kitchener',
            country: 'CA',
            weatherAPI: 'https://api.openweathermap.org/data/2.5/weather?q=',
            weatherAPIkey: '&appid=c747f431a398f65dfe5bb00ad7204547',
            outsideTemp: '0',
            home: 'My Home',
            weatherIcon: '',
            weather: '',
            weatherDesc: '',
            homeWasSetup: false,
            isError: false,
            errorTitle:'',
            errorMessage:'',
            hvacMode: 'Cool',
            fanMode: 'Auto',
            rooms: [],
        };

    }

    componentDidMount() {
        this.getWeather();
        this.getRoomInfo();
        this.ventiMainLoop();
        // this.weatherLoop();
        this.isHomeAlreadySetup().then(response => {
            console.log('Is home setup? ' + response.message);
            if (response.message !== true) {
                this.setState({homeWasSetup: true}, () => {
                    this.props.navigation.navigate('HomeSetupScreen');
                });
            } else {
                this.getHomeDetails().then(response => {
                    console.log('Home Details:' + response.message);
                    this.setState({home: response.message[0].home}, () => {
                        // this.ventiMainLoop();

                        console.log('My Home: ' + this.state.home);
                    });
                });
            }
        });
    }

    componentDidUpdate(prevProps) {
        if (!prevProps.isFocused && this.props.isFocused) {
            // this.ventiMainLoop();
        }
    }

    isHomeAlreadySetup = () => {
        return fetch(this.state.url + '/venti/isHomeSetup.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: 'empty',
        }).then((response) => response.json())
            .then((responseData) => {
                return responseData;
            }).catch(err => {
                this.errorHandler("Error from getWeather():",err.toString());
            })
    };

    updateTempSlider = (value, id) => {
        let rooms = this.state.rooms;
        let temp = Math.round(value * 2) / 2;
        for (let i = 0; i < rooms.length; i++) {
            if (rooms[i].id == id) {
                rooms[i].value = temp;
                break;
            }
        }
        this.setState({
            rooms: rooms,
        }, () => {
            this.updateSetTemp(id, temp);
        });
    };

    updateSetTemp = (id, temp) => {
        console.log(id, temp);
        fetch(this.state.url + '/venti/setTemp.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
            body: JSON.stringify({
                id: id,
                temp: temp,
            }),
        }).then((response) => response.json()).then((res) => {
            if (res.message === true) {
                // alert('Updated Temperature');
            } else {
                alert('Failed to set temperature');
            }
        }).done();
    };

    getHomeDetails = () => {
        return fetch(this.state.url + '/venti/getHomeDetails.php', {
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


    weatherLoop = () => {
        let timerId = setInterval(() => {
            this.getWeather();
        }, 60000);
    };

    ventiMainLoop = () => {

        let timerId = setInterval(() => {
            this.getRoomInfo();
        }, 5000);
    };

    getRoomInfo = () => {
        fetch(this.state.url + '/venti/getRoomInfo.php', {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
        }).then((response) => response.json()).then((res) => {
            let roomData = [];
            let setTempFunction = this.updateTempSlider;
            // let test = [setTempFunction()]
            for (let i = 0; i < res.message.length; i++) {
                console.log(res.message[i]);
                res.message[i]['test'] = setTempFunction;
                res.message[i]['value'] = res.message[i].setTemp;
                // res.message[i].concat(setTempFunction);
                roomData.push(res.message[i]);
            }

            // let roomData = Object.values(res.message);
            console.log(this.state.rooms);
            console.log(roomData);
            this.setState({rooms: roomData}, () => {
                this.getHomeDetails().then(response => {
                    console.log('Home Details:' + response.message);
                    this.setState({hvacMode: response.message[0].hvac, fanMode: response.message[0].fan}, () => {
                    });
                });
            });
        }).catch(err => {
            this.errorHandler("Error from getRoomInfo():",err.toString());
        }).done();
    };

    permission = async () => {
        try {
            const granted = await PermissionsAndroid.request(
                PermissionsAndroid.PERMISSIONS.ACCESS_FINE_LOCATION,
                {
                    title: 'Getting Location Permission',
                    message:
                        'Venti needs access to your location',
                    buttonNeutral: 'Ask Me Later',
                    buttonNegative: 'Cancel',
                    buttonPositive: 'OK',
                },
            );
            if (granted === PermissionsAndroid.RESULTS.GRANTED) {
                console.log('Permissions Granted');
            } else {
                console.log('Permissions Denied');
            }
        } catch (err) {
            console.warn(err);
        }
    };

    static navigationOptions = ({navigation}) => {
        const {state} = navigation;
        return {
            title: `${state.params.title}`,
        };
    };

    ChangeThisTitle = (titleText) => {
        const {setParams} = this.props.navigation;
        setParams({title: titleText});
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
        }).catch(err => {
            this.errorHandler("Error from setTemp():",err.toString());
        }).done();
    };
    // getTemp = () => {
    //     fetch(this.state.url + '/venti/getTemp.php', {
    //         method: 'POST',
    //         headers: {
    //             'Accept': 'application/json',
    //             'Content-Type': 'application/json',
    //         },
    //     }).then((response) => response.json()).then((res) => {
    //         let responseString = '';
    //         console.log(res);
    //         responseString += 'The temperatures are:\n\n';
    //         for (let i = 0; i < res.message.length; i++) {
    //             responseString += 'Vent: ' + res.message[i].mac + '\n';
    //             responseString += 'Temp: ' + res.message[i].temperature + '\n\n';
    //         }
    //         alert(responseString);
    //         console.log(res.message);
    //     }).catch(err => {
    //         this.errorHandler("Error from getRoomInfo():",err.toString());
    //     }).done();
    // };
    getWeather = () => {
        let weatherApiURL = this.state.weatherAPI + this.state.city + ',' + this.state.country + this.state.weatherAPIkey;
        fetch(weatherApiURL, {
            method: 'POST',
            headers: {
                'Accept': 'application/json',
                'Content-Type': 'application/json',
            },
        }).then((response) => response.json()).then((res) => {
            console.log(res);
            let temp = (res.main.temp - 275.15).toFixed(1);
            let weatherDesc = res.weather[0].description.charAt(0).toUpperCase() + res.weather[0].description.slice(1);
            this.setState({
                outsideTemp: temp,
                weatherIcon: res.weather[0].icon,
                weather: res.weather[0].main,
                weatherDesc: weatherDesc,
            }, () => {
                // this.updateWeatherInfo(temp);
                console.log(this.state.weatherDesc);
            });
        }).catch(err => {
            this.errorHandler("Error from getWeather():",err.toString());
        }).done();
    };

    errorHandler = (title, message) => {
        this.setState({
            isError: true,
            errorTitle: title,
            errorMessage: message
        }, ()=> {})
    }


    // updateWeatherInfo = (temp) => {
    //     let day = new Date().getDate();
    //     let month = new Date().getMonth();
    //     let year = new Date().getFullYear();
    //     let date = year + "/" + month + "/" + day;
    //
    //
    //     fetch(this.state.url + '/venti/updateHome.php', {
    //         method: 'POST',
    //         headers: {
    //             'Accept': 'application/json',
    //             'Content-Type': 'application/json',
    //         },
    //         body: JSON.stringify({
    //             date: date,
    //             temp: temp,
    //             home: this.state.home,
    //         }),
    //     }).then((response) => response.json()).then((res) => {
    //         if (res.message === true) {
    //             alert('Updated Info');
    //         } else {
    //             alert('Failed to update Info');
    //         }
    //     }).done();
    // }


    render() {
        if (this.props.route.params != undefined) {
            if (this.props.route.params.isFocused) {
                // this.ventiMainLoop();
            }
        }
        let coolMode = false;
        if (this.state.hvacMode === 'Cool') {
            coolMode = true;
        }

        return (
            <ScrollView>
                <Dialog.Container visible={this.state.isError}>
                    <Dialog.Title>{this.state.errorTitle}</Dialog.Title>
                    <Dialog.Description>
                        {this.state.errorMessage}
                    </Dialog.Description>
                    <Dialog.Button label="OK" onPress={() => this.setState({isError: false}, () => {
                    })}/>
                </Dialog.Container>
                <View style={{flex: 1, paddingTop: 20, paddingLeft: 3, backgroundColor: 'white'}}>
                    <View>
                        <Card title={this.state.home}>
                            <View style={{flexDirection: 'row'}}>
                                <View style={{flexDirection: 'column', flex: 10, paddingLeft: 10}}>
                                    <Text style={{paddingTop: 10}}>{this.state.weatherDesc}</Text>
                                    <Image style={{width: 50, height: 50, right: 10}}
                                           source={{uri: 'https://openweathermap.org/img/w/' + this.state.weatherIcon + '.png'}}/>
                                    {/*<Text style={{paddingTop: 10}}>{this.state.weather}</Text>*/}
                                    <Text style={{paddingTop: 0}}>{this.state.outsideTemp} °C</Text>
                                </View>
                                <View style={{flexDirection: 'column', flex: 4}}>
                                    <Text style={{paddingTop: 10}}>HVAC</Text>
                                    {
                                        coolMode ? <Icon name="snowflake" size={30} style={{top: 7, paddingBottom: 8}}
                                                         color="blue"/> :
                                            <Icon name="circle-off-outline" size={30} style={{top: 7, paddingBottom: 8}} color="#bbb"/>
                                    }
                                    <Text style={{paddingTop: 10}}>{this.state.hvacMode}</Text>
                                </View>
                                <View style={{flexDirection: 'column', flex: 4}}>
                                    <Text style={{paddingTop: 10}}>Fan</Text>
                                    <Icon name="fan" size={30} color="#bbb" style={{top: 7, paddingBottom: 8}}/>
                                    <Text style={{paddingTop: 10}}>{this.state.fanMode}</Text>
                                </View>

                    </View>
                        </Card>
                        <View>
                        </View>
                        <Card title={'My Rooms'}>
                            <FlatList
                                data={this.state.rooms}
                                renderItem={({item,index}) =>
                                    <TouchableOpacity style={[styles.item]}>
                                        <View style={{flexDirection: 'row', paddingBottom: 10, width: 200}}>
                                            <View style={{flexDirection: 'column', paddingRight: 50, width: 180}}>
                                                <Text style={{fontSize: 18}}>{item.name}</Text>
                                                <Text style={styles.title}>Currently: {item.temperature} °C</Text>
                                            </View>
                                            <View style={{flexDirection: 'column', paddingRight: 50, width: 180, marginLeft:-25}}>
                                                <Text style={{fontSize: 26}}>{item.value}°C</Text>
                                            </View>
                                        </View>
                                                <View style={{flexDirection:'row',paddingBottom:10,width:200}}>
                                                    <View style={{flexDirection: 'column', paddingRight: 10, width: 100}}>
                                                        <Slider
                                                            style={{width: 250, height: 40, marginLeft: -15}}
                                                            minimumValue={15}
                                                            value={this.state.rooms[index].value}
                                                            maximumValue={30}
                                                            minimumTrackTintColor="#0f437f"
                                                            maximumTrackTintColor="#333333"
                                                            thumbTintColor="#07A37f"
                                                            onValueChange={(value) => item.test(value, item.id)}
                                                        />
                                                    </View>
                                                </View>

                                    </TouchableOpacity>}
                                keyExtractor={(item) => item.id}
                                onPress={() => this.testing}
                            />
                        </Card>
                    </View>
                    {/*<View style={{paddingTop: 50}}>*/}
                    {/*    <Image style={{height: 100, width: 100}} source={require('../res/images/logo.png')}*/}
                    {/*    />*/}
                    {/*</View>*/}
                </View>
                <View style={{flex:1,paddingTop:150}}>
                <FAB buttonColor="#07ACDD" iconTextColor="#FFFFFF" onClickAction={() => {this.props.navigation.navigate('HomeSetupScreen');
                }} visible={true} iconTextComponent={<Icon name="plus"/>} />
                {/*<Button title="test" onPress={()=>SmartConfigP.stop()} />*/}
                </View>
                </ScrollView>
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
    item: {
        padding: 20,
        marginVertical: 8,
        marginHorizontal: 16,
        borderWidth: 2,
        borderRadius: 10,
        borderColor: '#dddddd',
    },
});


export default HomeScreen;
