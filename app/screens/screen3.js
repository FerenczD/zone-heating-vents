import React from 'react';
import {StyleSheet, Text, View} from 'react-native';

class ThirdScreen extends React.Component {
    render() {
        return (
            <View style={styles.container}>
                <Text>And One more</Text>
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

export default ThirdScreen;
