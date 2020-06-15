import React from 'react';
import {StyleSheet, Text, View} from 'react-native';
import ThirdScreen from './screen3';

class SecondScreen extends React.Component {
    render() {
        return (
            <View style={styles.container}>
                <Text>Some Other Screen</Text>
            </View>
        );
    }
}

const styles = StyleSheet.create({
    container: {
        flex: 1,
        justifyContent: 'center',
        alignItems: 'center',
    },
});

export default SecondScreen;
